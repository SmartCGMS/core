#include "calculate.h"


#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"
#include "descriptor.h" 

#include <iostream>

CCalculate_Filter::CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe) : mInput{ inpipe }, mOutput{ outpipe } {	
}


HRESULT CCalculate_Filter::Run(glucose::IFilter_Configuration* configuration)  {
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);

	mSignal_Id = shared_configuration.Read_GUID(rsSelected_Signal);
	mPrediction_Window = shared_configuration.Read_Double(rsPrediction_Window);

	for (; glucose::UDevice_Event evt = mInput.Receive(); evt) {

		switch (evt.event_code) {
			case glucose::NDevice_Event_Code::Level:
				Add_Level(evt.segment_id, evt.signal_id, evt.level, evt.device_time);
				Emit_Levels_At_Pending_Times(evt.segment_id);
				break;

			case glucose::NDevice_Event_Code::Parameters:
				if (evt.signal_id == mSignal_Id) {
					mWorking_Parameters.set(evt.parameters);	//make a deep copy as evt will be gone after this particular iteration
				}
				break;

			case glucose::NDevice_Event_Code::Parameters_Hint:
				if (evt.signal_id == mSignal_Id) Add_Parameters_Hint(evt.parameters);
				break;

			case glucose::NDevice_Event_Code::Recalculate_Segment:
				if ((evt.signal_id == Invalid_GUID) || (evt.signal_id == mSignal_Id)) {
					Reset_Segment(evt.segment_id);
					Emit_Levels_At_Pending_Times(evt.segment_id);
				}
		}


		if (!mOutput.Send(evt)) break;
	}

	return S_OK;
}

void CCalculate_Filter::Add_Level(const int64_t segment_id, const GUID &signal_id, const double level, const double time_stamp) {
	if ((signal_id == Invalid_GUID) || (signal_id == mSignal_Id)) return;	//cannot add what unknown signal and cannot add what we have to compute

	const auto &segment = mSegments[segment_id];
	if (segment) {
		if (segment->Add_Level(signal_id, level, time_stamp)) 
			mPending_Times.insert(time_stamp + mPrediction_Window);				
	}
}


void CCalculate_Filter::Emit_Levels_At_Pending_Times(const int64_t segment_id) {
	std::vector<double> levels, times{ mPending_Times.begin(), mPending_Times.end() };
	const auto &segment = mSegments[segment_id];
	if (segment) {
		if (segment->Calculate(mWorking_Parameters, times, levels)) {
			mPending_Times.clear();

			//send non-NaN values
			for (size_t i = 0; i < levels.size(); i++) {
				if (!isnan(levels[i])) {
					glucose::UDevice_Event calcEvt{ glucose::NDevice_Event_Code::Level };
					calcEvt.device_time = times[i];
					calcEvt.level = levels[i];
					calcEvt.device_id = calculate::Calculate_Filter_GUID;
					calcEvt.signal_id = mSignal_Id;
					calcEvt.segment_id = segment_id;
					mOutput.Send(calcEvt);
				}
				else
					mPending_Times.insert(times[i]);
			}
		}
	}
}

void CCalculate_Filter::Reset_Segment(const int64_t segment_id) {

}

void CCalculate_Filter::Set_Parameters(const int64_t segment_id, glucose::SModel_Parameter_Vector parameters) {
	mWorking_Parameters.set(parameters); //TODO thhis must be done per segment
}

void CCalculate_Filter::Add_Parameters_Hint(glucose::SModel_Parameter_Vector parameters) {
	glucose::SModel_Parameter_Vector hint;
	if (hint.set(parameters)) {
		mParameter_Hints.push_back(hint);	//push deep copy as the source may be gone unexpectedly
	}
}

/*

xxxxxxxxxxxxxxxxx

void CCalculate_Filter::Run_Main() {
	uint64_t segmentToReset = 0;
	bool error = false;

	double last_pending_time = std::numeric_limits<double>::quiet_NaN(); //TODO with more than one opened segment, this will fail

	CSegment_Holder hldr(mSignalId);

	for (;  glucose::UDevice_Event evt = mInput.Receive(); evt && !error) {
		switch (evt.event_code)
		{
			case glucose::NDevice_Event_Code::Level:
			case glucose::NDevice_Event_Code::Calibrated:
			{
				if (hldr.Add_Level(evt.segment_id, evt.signal_id, evt.device_time, evt.level)) {
					if (evt.device_time != last_pending_time) {
						mPending_Times.push_back(evt.device_time);	//this possibly create duplicities in the vector as it does not differentiate between measured signal - e.g. BG and IG measured at the very same time in animal experiment
						last_pending_time = evt.device_time;		//so we do this simple trick to avoid trivial repetitions, at least
					}


					std::vector<double> level;
					if (hldr.Get_Calculated_At_Time(evt.segment_id, mPending_Times, level)) {
						std::vector<double> repeatedly_pending_times(0);

						//send non-NaN values
						for (size_t i = 0; i < level.size(); i++) {
							if (!isnan(level[i])) {
								glucose::UDevice_Event calcEvt{ glucose::NDevice_Event_Code::Level };
								calcEvt.device_time = mPending_Times[i];
								calcEvt.level = level[i];
								calcEvt.device_id = Invalid_GUID; // TODO: fix this (retain from segments?)
								calcEvt.signal_id = mSignalId;
								calcEvt.segment_id = evt.segment_id;
								mOutput.Send(calcEvt);								
							} else
								repeatedly_pending_times.push_back(mPending_Times[i]);
						}

						mPending_Times = repeatedly_pending_times;
						if (mPending_Times.empty()) last_pending_time = std::numeric_limits<double>::quiet_NaN();
							else last_pending_time = mPending_Times[mPending_Times.size() - 1];

					}
				}

				break;
			}
			case glucose::NDevice_Event_Code::Time_Segment_Start:
				hldr.Start_Segment(evt.segment_id);
				break;
			case glucose::NDevice_Event_Code::Time_Segment_Stop:
				hldr.Stop_Segment(evt.segment_id);
				break;
			case glucose::NDevice_Event_Code::Parameters:
				if (evt.signal_id == mSignalId)
				{
					// if the segment doesn't have parameters yet, recalculate whole segment with given parameters (if the flag is set)
					if (mCalc_Past_With_First_Params)
						segmentToReset = evt.segment_id;
					hldr.Set_Parameters(evt.segment_id, evt.parameters);
				}
				break;
			case glucose::NDevice_Event_Code::Information:
				// if the reset is requested, defer sending levels AFTER propagating reset message
				if ((evt.info == rsParameters_Reset) && evt.signal_id == mSignalId)
					segmentToReset = evt.segment_id;
				break;
			default:
				break;
		}

		if (!mOutput.Send(evt))
			break;

		if (segmentToReset)
		{
			std::vector<double> times, levels;
			if (hldr.Calculate_All_Values(segmentToReset, times, levels))
			{
				mPending_Times.clear();
				last_pending_time = std::numeric_limits<double>::quiet_NaN();

				for (size_t i = 0; i < times.size(); i++) {
					//UDevice_Event is unique ptr, so we have to allocate it multiple times!
					if (!isnan(levels[i])) {

						glucose::UDevice_Event calcEvt{ glucose::NDevice_Event_Code::Level };
						calcEvt.device_id = GUID{ 0 }; // TODO: fix this (retain from segments?) - no, put there calc filter id
						calcEvt.signal_id = mSignalId;
						calcEvt.segment_id = segmentToReset;

						calcEvt.device_time = times[i];
						calcEvt.level = levels[i];

						if (!mOutput.Send(calcEvt))
						{
							// to properly break outer loop and avoid deadlock
							error = true;
							break;
						}
					}
				}

				if (!error)
				{
					glucose::UDevice_Event error_evt{ glucose::NDevice_Event_Code::Information };
					error_evt.info.set(rsSegment_Recalculate_Complete);

					if (!mOutput.Send(error_evt))
						break;
				}
			}

			segmentToReset = 0;
		}
	}
}

*/