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
		// just read and do nothing - this effectively consumes any incoming event through pipe
		glucose::Release_Event(evt);	
}

HRESULT CConsume_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	// just start thread, this filter does not have any parameters (yet)

	Run_Main();

	return S_OK;
};
