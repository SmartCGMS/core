#include "calculate.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include "Segment_Holder.h"

#include <iostream>

CCalculate_Filter::CCalculate_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
	: mInput(inpipe), mOutput(outpipe), mSignalId{0}, mCalc_Past_With_First_Params(false)
{
	//
}

void CCalculate_Filter::Run_Main()
{
	glucose::TDevice_Event evt, calcEvt;
	uint64_t segmentToReset = 0;
	bool error = false;

	CSegment_Holder hldr(mSignalId);

	while (mInput->receive(&evt) == S_OK && !error)
	{
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
						calcEvt.device_time = evt.device_time;
						calcEvt.event_code = glucose::NDevice_Event_Code::Level;
						calcEvt.level = level;
						//calcEvt.logical_time = 0; // asynchronnous events always have logical time equal to 0 at this time
						calcEvt.device_id = GUID{ 0 }; // TODO: fix this (retain from segments?)
						calcEvt.signal_id = mSignalId;
						calcEvt.segment_id = evt.segment_id;
						mOutput->send(&calcEvt);
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
					hldr.Set_Parameters(evt.segment_id, refcnt::make_shared_reference_ext<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(evt.parameters, true));
				}
				break;
			case glucose::NDevice_Event_Code::Information:
				// if the reset is requested, defer sending levels AFTER propagating reset message
				if (refcnt::WChar_Container_Equals_WString(evt.info, rsParameters_Reset) && evt.signal_id == mSignalId)
					segmentToReset = evt.segment_id;
				break;
			default:
				break;
		}

		if (mOutput->send(&evt) != S_OK)
			break;

		if (segmentToReset)
		{
			std::vector<double> times, levels;
			if (hldr.Calculate_All_Values(segmentToReset, times, levels))
			{
				calcEvt.event_code = glucose::NDevice_Event_Code::Level;
				//calcEvt.logical_time = 0; // asynchronnous events always have logical time equal to 0 at this time
				calcEvt.device_id = GUID{ 0 }; // TODO: fix this (retain from segments?)
				calcEvt.signal_id = mSignalId;
				calcEvt.segment_id = segmentToReset;

				for (size_t i = 0; i < times.size(); i++)
				{
					calcEvt.device_time = times[i];
					calcEvt.level = levels[i];

					if (mOutput->send(&calcEvt) != S_OK)
					{
						// to properly break outer loop and avoid deadlock
						error = true;
						break;
					}
				}

				if (!error)
				{
					calcEvt.event_code = glucose::NDevice_Event_Code::Information;
					calcEvt.info = refcnt::WString_To_WChar_Container(rsSegment_Recalculate_Complete);

					if (mOutput->send(&calcEvt) != S_OK)
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

	if (mSignalId == GUID{ 0 })
		return E_FAIL;

	Run_Main();

	return S_OK;
};
