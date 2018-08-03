#include "calculate.h"


#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <iostream>

CCalculate_Filter::CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe) : mInput{ inpipe }, mOutput{ outpipe } {	
}
std::unique_ptr<CTime_Segment>& CCalculate_Filter::Get_Segment(const int64_t segment_id) {
	const auto iter = mSegments.find(segment_id);

	if (iter != mSegments.end()) return iter->second;
	else {
		std::unique_ptr<CTime_Segment> segment = std::make_unique<CTime_Segment>(segment_id, mSignal_Id, mPrediction_Window, mOutput);		
		const auto ret = mSegments.insert(std::pair<int64_t, std::unique_ptr<CTime_Segment>>{segment_id, std::move(segment)});
		return ret.first->second;
	}
}

HRESULT CCalculate_Filter::Run(glucose::IFilter_Configuration* configuration)  {
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);

	mSignal_Id = shared_configuration.Read_GUID(rsSelected_Signal);
	mPrediction_Window = shared_configuration.Read_Double(rsPrediction_Window);
	mSolver_Enabled = shared_configuration.Read_Bool(rsSolve_Parameters);
	mSolve_On_Calibration = shared_configuration.Read_Bool(rsSolve_On_Level_Count);
	mReference_Level_Threshold_Count = shared_configuration.Read_Int(rsSolve_On_Level_Count);
	mSolving_Scheduled = false;
	mReference_Level_Counter = 0;

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {

		bool event_already_sent = false;

		switch (evt.event_code) {
			case glucose::NDevice_Event_Code::Level:
				{					
					//copy those values, which may be gone once we send the event in the original order
					const int64_t segment_id = evt.segment_id;
					const GUID signal_id = evt.signal_id;
					const double level = evt.level;
					const double device_time = evt.device_time;

					Schedule_Solving(signal_id);

					//send the original event before other events are emitted
					if (mOutput.Send(evt)) {
						//now, evt may be gone!
						event_already_sent = true;
						Add_Level(segment_id, signal_id, level, device_time);					
					}
					else
						break;
				}
				break;

			case glucose::NDevice_Event_Code::Parameters:
				if (evt.signal_id == mSignal_Id) {
					const auto &segment = Get_Segment(evt.segment_id);
					if (segment) {
						segment->Set_Parameters(evt.parameters);
						Add_Parameters_Hint(evt.parameters);
					}
				}
				break;

			case glucose::NDevice_Event_Code::Parameters_Hint:
				if (evt.signal_id == mSignal_Id) Add_Parameters_Hint(evt.parameters);
				break;

			case glucose::NDevice_Event_Code::Solve_Parameters:
				Run_Solver();
				break;

			case glucose::NDevice_Event_Code::Warm_Reset:
				for (const auto &segment : mSegments)
					segment.second->Clear_Data();
				break;				
		}


		if (!event_already_sent)
			if (!mOutput.Send(evt)) 
					break;
	}

	return S_OK;
}

void CCalculate_Filter::Add_Level(const int64_t segment_id, const GUID &signal_id, const double level, const double time_stamp) {
	
	if ((signal_id == Invalid_GUID) || (signal_id == mSignal_Id)) return;	//cannot add what unknown signal and cannot add what we have to compute

	const auto &segment = Get_Segment(segment_id);
	if (segment)
		if (segment->Add_Level(signal_id, level, time_stamp)) {
			if (mSolving_Scheduled) Run_Solver();	
			segment->Emit_Levels_At_Pending_Times();
		}
}


void CCalculate_Filter::Add_Parameters_Hint(glucose::SModel_Parameter_Vector parameters) {
	glucose::SModel_Parameter_Vector hint;
	if (hint.set(parameters)) {
		mParameter_Hints.push_back(hint);	//push deep copy as the source may be gone unexpectedly
	}
}

void CCalculate_Filter::Schedule_Solving(const GUID &level_signal_id) {
	mSolving_Scheduled = false;

	if (level_signal_id == mSignal_Id) mReference_Level_Counter++;
	if ((mReference_Level_Threshold_Count > 0) && (mReference_Level_Counter >= mReference_Level_Threshold_Count)) {
		mReference_Level_Counter = 0;
		mSolving_Scheduled = true;
	}

	if ((level_signal_id == glucose::signal_Calibration) && mSolve_On_Calibration) mSolving_Scheduled = true;
}

void CCalculate_Filter::Run_Solver() {
	/*TODO taky doimplementovat nacitani defaultnich parametru, aby je pouzival jak solver, tak i calculator
		a jeste konfigurovatelne meze pro solver

	{
		double *pb, *pe;
		if (cur->parameters->get(&pb, &pe) == S_OK)
		{
			size_t paramcnt = std::distance(pb, pe) / 3; // lower, defaults, upper
			lowBound = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(pb, pb + paramcnt);
			defParams = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(pb + paramcnt, pb + 2 * paramcnt);
			highBound = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(pb + 2 * paramcnt, pe);
		}

		*/
}