/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "solver.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include "Compute_Holder.h"
#include "descriptor.h"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <string>

CCompute_Filter::CCompute_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput(inpipe), mOutput(outpipe), mRecalcLevelsCount(0), mRecalcSegmentEnd(false), mRecalcOnCalibration(false), mHold_During_Solve(false), mTotalLevelCount(0),
	  mLastCalculationLevelsCount(0), mCalculationScheduled(false), mCalculationForced(false), mParamsResetForced(false), mSuspended(false)
{
	//
}

void CCompute_Filter::Run_Main()
{
	bool resetFlag = false;

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {

		// also do not shutdown during solve
		if (mHold_During_Solve || evt.event_code == glucose::NDevice_Event_Code::Shut_Down)
		{
			std::unique_lock<std::mutex> lck(mScheduleMtx);

			if (mComputeHolder->Is_Solve_In_Progress())
				mScheduleCv.wait(lck, [this]() { return !mComputeHolder->Is_Solve_In_Progress(); });
		}
	
		switch (evt.event_code)
		{
			// incoming level or calibration - find appropriate signal and add new level
			case glucose::NDevice_Event_Code::Level:
			{
				if (mComputeHolder->Add_Level(evt.segment_id, evt.signal_id, evt.device_time, evt.level))
				{
					mTotalLevelCount++;

					// recalculate when 1) configured level count has been reached or 2) calibration level came
					if ((mRecalcLevelsCount > 0 && mTotalLevelCount >= mLastCalculationLevelsCount + mRecalcLevelsCount) ||
						(evt.signal_id == glucose::signal_Calibration && mRecalcOnCalibration))
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
				if (evt.signal_id == Invalid_GUID || evt.signal_id == mComputeHolder->Get_Signal_Id())
				{
					std::unique_lock<std::mutex> lck(mScheduleMtx);
					mSuspended = true;
				}
				break;
			// resume parameter solving - solver threads will be started again; if there's some pending solve, start it immediatelly
			case glucose::NDevice_Event_Code::Resume_Parameter_Solving:
				if (evt.signal_id == Invalid_GUID || evt.signal_id == mComputeHolder->Get_Signal_Id())
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
				if (evt.info == rsParameters_Reset_Request && (evt.signal_id == mComputeHolder->Get_Signal_Id() || evt.signal_id == Invalid_GUID))
					resetFlag = true;
				break;
			default:
				break;
		}

		if (!mOutput.Send(evt))
			break;
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

				bool resetForcedFlag = mParamsResetForced;
				mCalculationForced = false;

				// reset parameters if requested
				if (resetForcedFlag)
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
					std::vector<uint64_t> updatedSegments;
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
						evt.device_id = compute::solver_id;
						evt.signal_id = mComputeHolder->Get_Signal_Id();

						evt.segment_id = segId;

						evt.parameters.set(mComputeHolder->Get_Model_Parameters(segId));
						error = !mOutput.Send(evt);
						if (error)
							break;
					}

					// send also parameters reset information message
					if ((resetForcedFlag || mRecalc_With_Every_Params) && !error)
					{
						for (auto& segId : updatedSegments)
						{
							if (!mComputeHolder->Get_Model_Parameters(segId))
								continue;

							glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Information };
							evt.device_time = mComputeHolder->Get_Max_Time();
							evt.device_id = compute::solver_id;
							evt.signal_id = mComputeHolder->Get_Signal_Id();

							evt.segment_id = segId;

							evt.info.set(rsParameters_Reset);
							if (!mOutput.Send(evt))
								break;
						}
					}
				}
				else // send solver failed message
				{
					glucose::UDevice_Event failed_evt{ glucose::NDevice_Event_Code::Information };
					failed_evt.device_time = Unix_Time_To_Rat_Time(time(nullptr));
					failed_evt.device_id = compute::solver_id;
					failed_evt.signal_id = mComputeHolder->Get_Signal_Id();

					std::wstring progMsg = rsInfo_Solver_Failed;

					failed_evt.info.set(progMsg.c_str());
					failed_evt.signal_id = mComputeHolder->Get_Signal_Id();
					mOutput.Send(failed_evt);
				}

				// explicitly lock again, so we could notify main thread if needed
				lck.lock();
				mScheduleCv.notify_all();
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

HRESULT CCompute_Filter::Run(glucose::IFilter_Configuration* configuration) {
	// temporarily stored arguments due to need of constructing holder using const initializers
	GUID solver_id, signal_id, metric_id, model_id;
	unsigned char relative_error = false, squared_diff = true, prefer_more = false, use_measured = true;
	double metric_threshold = 0.0;
	bool use_just_opened = false;
	size_t levels_required = 1;

	glucose::SModel_Parameter_Vector lowBound, defParams, highBound;

	auto conf = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);

	solver_id = conf.Read_GUID(rsSelected_Solver);
	model_id = conf.Read_GUID(rsSelected_Model);
	signal_id = conf.Read_GUID(rsSelected_Signal);
	metric_id = conf.Read_GUID(rsSelected_Metric);
	relative_error = conf.Read_Bool(rsUse_Relative_Error);
	squared_diff = conf.Read_Bool(rsUse_Squared_Diff);
	prefer_more = conf.Read_Bool(rsUse_Prefer_More_Levels);
	use_measured = conf.Read_Bool(rsUse_Measured_Levels);
	metric_threshold = conf.Read_Double(rsMetric_Threshold);
	levels_required = static_cast<size_t>(conf.Read_Int(rsMetric_Levels_Required));
	mRecalcLevelsCount = conf.Read_Int(rsRecalculate_On_Levels_Count);
	mRecalcSegmentEnd = conf.Read_Bool(rsRecalculate_On_Segment_End);
	mRecalcOnCalibration = conf.Read_Bool(rsRecalculate_On_Calibration);
	mRecalc_With_Every_Params = conf.Read_Bool(rsRecalculate_With_Every_Params);
	use_just_opened = conf.Read_Bool(rsUse_Just_Opened_Segments);
	mHold_During_Solve = conf.Read_Bool(rsHold_During_Solve);

	std::vector<double> mergedBounds = conf.Read_Double_Array(rsSelected_Model_Bounds);
	// valid bounds container element count is a multiple of 3
	if (!mergedBounds.empty() && (mergedBounds.size() % 3) == 0)
	{
		double* base = mergedBounds.data();
		size_t paramcnt = mergedBounds.size() / 3; // lower, defaults, upper
		lowBound = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(&base[0], &base[paramcnt]);
		defParams = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(&base[paramcnt], &base[2 * paramcnt]);
		highBound = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(&base[2 * paramcnt], &base[3 * paramcnt]);
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
