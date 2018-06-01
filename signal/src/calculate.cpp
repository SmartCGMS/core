#include "calculate.h"


#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include "Segment_Holder.h"

#include <iostream>

CCalculate_Filter::CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }, mSignalId{ Invalid_GUID },
	  mRecalculate_Past_On_Params(false), mRecalculate_Past_On_Segment_Stop(false)
{
	//
}


HRESULT CCalculate_Filter::Run(glucose::IFilter_Configuration* configuration)  {
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);

	mSignalId = shared_configuration.Read_GUID(rsSelected_Signal);
	mRecalculate_Past_On_Params = shared_configuration.Read_Bool(rsRecalculate_Past_On_Params);
	mRecalculate_Past_On_Segment_Stop = shared_configuration.Read_Bool(rsRecalculate_Past_On_Segment_Stop);

	if (mSignalId == Invalid_GUID)
		return E_FAIL;

	CSegment_Holder segments{ mSignalId, mOutput };

	for (; glucose::UDevice_Event evt = mInput.Receive(); evt) {

		switch (evt.event_code) {
			case glucose::NDevice_Event_Code::Level:
			case glucose::NDevice_Event_Code::Calibrated:
				segments.Add_Level(evt.level, evt.device_time, evt.signal_id, evt.segment_id);
				segments.Emit_Levels_At_Pending_Times(evt.segment_id);
				break;

			case glucose::NDevice_Event_Code::Parameters:
				if (evt.signal_id == mSignalId) {
					segments.Set_Parameters(evt.parameters, evt.segment_id);
					if (mRecalculate_Past_On_Params) {
						segments.Reset_Segment(evt.segment_id);
						segments.Emit_Levels_At_Pending_Times(evt.segment_id);
					}
				}
				break;

			case glucose::NDevice_Event_Code::Parameters_Hint:
				if (evt.signal_id == mSignalId) segments.Add_Parameters_Hint(evt.parameters);
				break;

			case glucose::NDevice_Event_Code::Recalculate_Segment:
				if ((evt.signal_id != Invalid_GUID) && (evt.signal_id != mSignalId)) break;
					//partial break

			case glucose::NDevice_Event_Code::Time_Segment_Stop:			
				//if asked for, recalculate entire segment
				if (mRecalculate_Past_On_Segment_Stop) {
					segments.Reset_Segment(evt.segment_id);
					segments.Emit_Levels_At_Pending_Times(evt.segment_id);
				}
				break;
		}


		if (!mOutput.Send(evt)) break;
	}

	return S_OK;
}


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

