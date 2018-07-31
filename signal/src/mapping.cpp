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

HRESULT CMapping_Filter::Run(glucose::IFilter_Configuration* configuration) {
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);
	mSourceId = shared_configuration.Read_GUID(rsSignal_Source_Id);
	mDestinationId = shared_configuration.Read_GUID(rsSignal_Destination_Id);

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
		if (evt.signal_id == mSourceId)
			evt.signal_id = mDestinationId;

		if (!mOutput.Send(evt))
			break;
	}

	return S_OK;
};
