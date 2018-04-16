#include "consume.h"

#include "../../../common/rtl/FilterLib.h"

#include <iostream>
#include <chrono>

CConsume_Filter::CConsume_Filter(glucose::IFilter_Pipe* inpipe)
	: mInput(inpipe)
{
	//
}

void CConsume_Filter::Run_Main()
{
	glucose::TDevice_Event evt;

	while (mInput->receive(&evt) == S_OK)
	{
		// just read and do nothing - this effectively consumes any incoming event through pipe

		// release references on container objects
		switch (evt.event_code)
		{
			case glucose::NDevice_Event_Code::Information:
			case glucose::NDevice_Event_Code::Warning:
			case glucose::NDevice_Event_Code::Error:
				evt.info->Release();
				break;
			case glucose::NDevice_Event_Code::Parameters:
			case glucose::NDevice_Event_Code::Parameters_Hint:
				evt.parameters->Release();
				break;
			default:
				break;
		}
	}
}

HRESULT CConsume_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	// just start thread, this filter does not have any parameters (yet)

	Run_Main();

	return S_OK;
};
