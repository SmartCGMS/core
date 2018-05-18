#include "mapping.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <iostream>
#include <chrono>

CMapping_Filter::CMapping_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }
{
	//
}

void CMapping_Filter::Run_Main() {

	for (glucose::UDevice_Event evt = mInput.Receive(); ; evt) {

		// remap only "Level" signals
		if (evt.event_code == glucose::NDevice_Event_Code::Level)
		{
			// map source to destination ID
			if (evt.signal_id == mSourceId)
				evt.signal_id = mDestinationId;
		}
		// remap parameters reset information message (this message serves i.e. for drawing filter to reset signal values)
		else if (evt.event_code == glucose::NDevice_Event_Code::Information)
		{
			if (evt.signal_id == mSourceId && evt.info == rsParameters_Reset)
				evt.signal_id = mDestinationId;
		}

		if (!mOutput.Send(evt))
			break;
	}
}

HRESULT CMapping_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
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

		if (confname == rsSignal_Source_Id)
			mSourceId = cur->guid;
		else if (confname == rsSignal_Destination_Id)
			mDestinationId = cur->guid;
	}

	Run_Main();

	return S_OK;
};
