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

	CSegment_Holder hldr(mSignalId);

	for (glucose::UDevice_Event evt = mInput.Receive(); ; evt && !error) {
		switch (evt.event_code)
		{
			case glucose::NDevice_Event_Code::Level:
			case glucose::NDevice_Event_Code::Calibrated:
			{
				if (hldr.Add_Level(evt.segment_id, evt.signal_id, evt.device_time, evt.level))
				{
					double level;
					if (hldr.Get_Calculated_At_Time(evt.segment_id, evt.device_time, level))
					{
						glucose::UDevice_Event calcEvt{ glucose::NDevice_Event_Code::Level };
						calcEvt.device_time = evt.device_time;						
						calcEvt.level = level;
						calcEvt.device_id = Invalid_GUID; // TODO: fix this (retain from segments?)
						calcEvt.signal_id = mSignalId;
						calcEvt.segment_id = evt.segment_id;
						mOutput.Send(calcEvt);
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
					if (!hldr.Has_Parameters(evt.segment_id) && mCalc_Past_With_First_Params)
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
				glucose::UDevice_Event calcEvt{ glucose::NDevice_Event_Code::Level };
				//calcEvt.logical_time = 0; // asynchronnous events always have logical time equal to 0 at this time
				calcEvt.device_id = GUID{ 0 }; // TODO: fix this (retain from segments?)
				calcEvt.signal_id = mSignalId;
				calcEvt.segment_id = segmentToReset;

				for (size_t i = 0; i < times.size(); i++)
				{
					calcEvt.device_time = times[i];
					calcEvt.level = levels[i];

					if (!mOutput.Send(calcEvt))
					{
						// to properly break outer loop and avoid deadlock
						error = true;
						break;
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

HRESULT CCalculate_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
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
