#include "calculate.h"


#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include "Segment_Holder.h"

#include <iostream>

CCalculate_Filter::CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }, mSignalId{ 0 }, mCalc_Past_With_First_Params(false)
{
	//
}


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

HRESULT CCalculate_Filter::Run(refcnt::IVector_Container<glucose::TFilter_Parameter>* const configuration)
{
	glucose::TFilter_Parameter *cbegin, *cend;
	if (configuration->get(&cbegin, &cend) != S_OK)
		return E_FAIL;

	for (glucose::TFilter_Parameter* cur = cbegin; cur < cend; cur += 1)
	{
		wchar_t *begin, *end;
		if (cur->config_name->get(&begin, &end) != S_OK)
			continue;

		std::wstring confname{ begin, end };

		if (confname == rsSelected_Signal)
			mSignalId = cur->guid;
		else if (confname == rsCalculate_Past_New_Params)
			mCalc_Past_With_First_Params = cur->boolean;
	}

	if (mSignalId == Invalid_GUID)
		return E_FAIL;

	Run_Main();

	return S_OK;
};
