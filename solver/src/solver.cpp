#include "solver.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include "Compute_Holder.h"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>

CCompute_Filter::CCompute_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput(inpipe), mOutput(outpipe), mTotalLevelCount(0), mLastCalculationLevelsCount(0), mCalculationScheduled(false),
	  mRecalcLevelsCount(0), mRecalcSegmentEnd(false), mRecalcOnCalibration(false), mSuspended(false), mCalculationForced(false),
	  mParamsResetForced(false)
{
	//
}

void CCompute_Filter::Run_Main()
{
	

	double maxTime = 0.0;
	bool resetFlag = false;

	for (; glucose::UDevice_Event evt = mInput.Receive(); evt) {
	
		switch (evt.event_code)
		{
			// incoming level or calibration - find appropriate signal and add new level
			case glucose::NDevice_Event_Code::Level:
			case glucose::NDevice_Event_Code::Calibrated:
			{
				if (mComputeHolder->Add_Level(evt.segment_id, evt.signal_id, evt.device_time, evt.level))
				{
					mTotalLevelCount++;

					// recalculate when 1) configured level count has been reached or 2) calibration level came
					if ((mRecalcLevelsCount > 0 && mTotalLevelCount >= mLastCalculationLevelsCount + mRecalcLevelsCount) ||
						(evt.event_code == glucose::NDevice_Event_Code::Calibrated && mRecalcOnCalibration))
					{
						std::unique_lock<std::mutex> lck(mScheduleMtx);
						mCalculationScheduled = true;
						mScheduleCv.notify_all();
					}
				}

				break;
			}
			// parameters hint - incoming hints for solution, store to compute holder
			case glucose::NDevice_Event_Code::Parameters_Hint:
				if (evt.signal_id == mComputeHolder->Get_Signal_Id())
					mComputeHolder->Add_Solution_Hint(evt.parameters);
				break;
			// time segment start - create a new segment
			case glucose::NDevice_Event_Code::Time_Segment_Start:
				mComputeHolder->Start_Segment(evt.segment_id);
				if (glucose::SModel_Parameter_Vector params = mComputeHolder->Get_Model_Parameters(evt.segment_id))
				{
					// send current event through pipe
					//if (!mOutput.Send(evt)) break; - will be done later

					glucose::UDevice_Event modified_evt{ glucose::NDevice_Event_Code::Parameters };
					modified_evt.device_time = mComputeHolder->Get_Max_Time();
					modified_evt.signal_id = mComputeHolder->Get_Signal_Id();
					modified_evt.parameters = params;

					// modify the event that will be sent through pipe
					if (!mOutput.Send(modified_evt)) break;
				}
				break;
			// time segment stop - force buffered values to be written to signals
			case glucose::NDevice_Event_Code::Time_Segment_Stop:
				mComputeHolder->Stop_Segment(evt.segment_id);
				if (mRecalcSegmentEnd)
				{
					std::unique_lock<std::mutex> lck(mScheduleMtx);
					mCalculationScheduled = true;
					mScheduleCv.notify_all();
				}
				break;
			// suspend parameter solving - no solver thread will start until "resume" message
			case glucose::NDevice_Event_Code::Suspend_Parameter_Solving:
				if (evt.signal_id == GUID{ 0 } || evt.signal_id == mComputeHolder->Get_Signal_Id())
				{
					std::unique_lock<std::mutex> lck(mScheduleMtx);
					mSuspended = true;
				}
				break;
			// resume parameter solving - solver threads will be started again; if there's some pending solve, start it immediatelly
			case glucose::NDevice_Event_Code::Resume_Parameter_Solving:
				if (evt.signal_id == GUID{ 0 } || evt.signal_id == mComputeHolder->Get_Signal_Id())
				{
					std::unique_lock<std::mutex> lck(mScheduleMtx);
					mSuspended = false;
					mScheduleCv.notify_all();
				}
				break;
			// force parameters solve
			case glucose::NDevice_Event_Code::Solve_Parameters:
				if ((evt.signal_id == Invalid_GUID) || (evt.signal_id == mComputeHolder->Get_Signal_Id())) {
					std::unique_lock<std::mutex> lck(mScheduleMtx);
					mCalculationScheduled = true;
					mCalculationForced = true;
					mParamsResetForced = resetFlag;
					resetFlag = false;
					mScheduleCv.notify_all();
				}
				break;
			// information messages (misc)
			case glucose::NDevice_Event_Code::Information:
				if (evt.info == rsParameters_Reset_Request)
					resetFlag = true;
				break;
			default:
				break;
		}

		if (!mOutput.Send(evt) ) break;		
	}

	mRunning = false;

	// lock scope
	{
		std::unique_lock<std::mutex> lck(mScheduleMtx);

		// abort solver progress, if there's a calculation running
		if (mComputeHolder->Is_Solve_In_Progress())
			mComputeHolder->Abort();

		mScheduleCv.notify_all();
	}

	// join scheduler thread until it's finished
	if (mSchedulerThread && mSchedulerThread->joinable())
		mSchedulerThread->join();
}

void CCompute_Filter::Run_Scheduler()
{
	while (mRunning)
	{
		std::unique_lock<std::mutex> lck(mScheduleMtx);

		if (!mComputeHolder->Is_Solve_In_Progress() && mCalculationScheduled && (!mSuspended || mCalculationForced))
		{
			// if there's something new, solve
			if (mTotalLevelCount != mLastCalculationLevelsCount || mCalculationForced)
			{
				mLastCalculationLevelsCount = mTotalLevelCount;

				bool forcedFlag = mCalculationForced;
				bool resetForcedFlag = mParamsResetForced;
				mCalculationForced = false;

				// reset parameters if requested
				if (mParamsResetForced)
				{
					mComputeHolder->Reset_Model_Parameters();
					mParamsResetForced = false;
				}

				auto solverResult = mComputeHolder->Solve_Async();

				mCalculationScheduled = false;

				// unlock the lock, because at this time, the segment was copied, so no damage is possible
				lck.unlock();

				std::future_status status;
				size_t lastProgress = (size_t)-1, newProgress;
				do
				{
					status = solverResult.wait_for(std::chrono::milliseconds(500));

					newProgress = mComputeHolder->Get_Solve_Percent_Complete();

					// if the progress value changed, update
					if (lastProgress != newProgress)
					{
						glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Information };
						evt.signal_id = mComputeHolder->Get_Signal_Id();

						std::wstring progMsg = rsInfo_Solver_Progress;
						progMsg += L"=";
						progMsg += std::to_wstring(mComputeHolder->Get_Solve_Percent_Complete());

						evt.info.set(progMsg.c_str());							
						evt.signal_id = mComputeHolder->Get_Signal_Id();
						mOutput.Send(evt);

						lastProgress = newProgress;
					}

				} while (status != std::future_status::ready);

				auto result = solverResult.get();

				// new or better parameters available
				if (result == S_OK)
				{
					std::vector<size_t> updatedSegments;
					if (resetForcedFlag || mRecalc_With_Every_Params)
						mComputeHolder->Get_All_Segment_Ids(updatedSegments);
					else
						mComputeHolder->Get_Improved_Segments(updatedSegments);

				

					bool error = false;

					for (auto& segId : updatedSegments)
					{
						if (!mComputeHolder->Get_Model_Parameters(segId))
							continue;

						glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Parameters };
						evt.device_time = mComputeHolder->Get_Max_Time();
						evt.device_id = Invalid_GUID; // TODO: fix this (retain from segments?) 
						evt.signal_id = mComputeHolder->Get_Signal_Id();

						evt.segment_id = segId;

						// send also parameters reset information message						
						evt.parameters = mComputeHolder->Get_Model_Parameters(segId);									
						error = !mOutput.Send(evt);
						if (error) break;
					}

					if ((resetForcedFlag || mRecalc_With_Every_Params) && !error)
					{
						for (auto& segId : updatedSegments)
						{
							if (!mComputeHolder->Get_Model_Parameters(segId))
								continue;

							glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Information };
							evt.device_time = mComputeHolder->Get_Max_Time();
							evt.device_id = Invalid_GUID; // TODO: fix this (retain from segments?) 
							evt.signal_id = mComputeHolder->Get_Signal_Id();

							evt.segment_id = segId;

							evt.info.set(rsParameters_Reset);
							if (!mOutput.Send(evt))  break;
						}
					}
				}
			}
		}
		else
		{
			// do not go to sleep when the calculation was scheduled; however, if the solver is suspended, sleep
			if (!mCalculationScheduled || mSuspended)
				mScheduleCv.wait(lck);
		}
	}
}

HRESULT CCompute_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	// temporarily stored arguments due to need of constructing holder using const initializers
	GUID solver_id, signal_id, metric_id, model_id;
	unsigned char relative_error, squared_diff, prefer_more, use_measured;
	double metric_threshold;
	bool use_just_opened;
	size_t levels_required;

	glucose::SModel_Parameter_Vector lowBound, defParams, highBound;

	glucose::TFilter_Parameter *cbegin, *cend;
	if (configuration->get(&cbegin, &cend) != S_OK)
		return E_FAIL;

	for (glucose::TFilter_Parameter* cur = cbegin; cur < cend; cur += 1)
	{
		wchar_t *begin, *end;
		if (cur->config_name->get(&begin, &end) != S_OK)
			continue;

		std::wstring confname{ begin, end };

		if (confname == rsSelected_Solver)
			solver_id = cur->guid;
		else if (confname == rsSelected_Model)
			model_id = cur->guid;
		else if (confname == rsSelected_Signal)
			signal_id = cur->guid;
		else if (confname == rsSelected_Metric)
			metric_id = cur->guid;
		else if (confname == rsUse_Relative_Error)
			relative_error = cur->boolean;
		else if (confname == rsUse_Squared_Diff)
			squared_diff = cur->boolean;
		else if (confname == rsUse_Prefer_More_Levels)
			prefer_more = cur->boolean;
		else if (confname == rsUse_Measured_Levels)
			use_measured = cur->boolean;
		else if (confname == rsMetric_Threshold)
			metric_threshold = cur->dbl;
		else if (confname == rsMetric_Levels_Required)
			levels_required = static_cast<size_t>(cur->int64);
		else if (confname == rsRecalculate_On_Levels_Count)
			mRecalcLevelsCount = cur->int64;
		else if (confname == rsRecalculate_On_Segment_End)
			mRecalcSegmentEnd = cur->boolean;
		else if (confname == rsRecalculate_On_Calibration)
			mRecalcOnCalibration = cur->boolean;
		else if (confname == rsRecalculate_With_Every_Params)
			mRecalc_With_Every_Params = cur->boolean;
		else if (confname == rsUse_Just_Opened_Segments)
			use_just_opened = cur->boolean;
		else if (confname == rsSelected_Model_Bounds)
		{
			double *pb, *pe;
			if (cur->parameters->get(&pb, &pe) == S_OK)
			{
				size_t paramcnt = std::distance(pb, pe) / 3; // lower, defaults, upper
				lowBound = refcnt::Create_Container_shared<double>(pb, pb + paramcnt);
				defParams = refcnt::Create_Container_shared<double>(pb + paramcnt, pb + 2* paramcnt);
				highBound = refcnt::Create_Container_shared<double>(pb + 2*paramcnt, pe);
			}
		}
	}

	glucose::TMetric_Parameters metric_params{ metric_id, relative_error, squared_diff, prefer_more, metric_threshold };

	mRunning = true;

	mComputeHolder = std::make_unique<CCompute_Holder>(solver_id, signal_id, metric_params, levels_required, use_measured, use_just_opened);
	mComputeHolder->Set_Bounds(lowBound, highBound);
	mComputeHolder->Set_Defaults(defParams);

	mSchedulerThread = std::make_unique<std::thread>(&CCompute_Filter::Run_Scheduler, this);
	
	Run_Main();

	return S_OK;
};
