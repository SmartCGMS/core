#include "interop-export.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/rattime.h"

#include <chrono>

TInterop_Listener _registeredListener = nullptr;

CInterop_Export_Filter::CInterop_Export_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
	: mInput(inpipe), mOutput(outpipe), mLogicalTime(0)
{
	//
}

CInterop_Export_Filter::~CInterop_Export_Filter()
{
	//
}

void CInterop_Export_Filter::Run_Main()
{
	glucose::TDevice_Event evt;

	while (mInput->receive(&evt) == S_OK)
	{
		// if there was a listener registered, pass the event
		if (_registeredListener)
		{
			_registeredListener(
				static_cast<uint8_t>(evt.event_code),
				reinterpret_cast<uint8_t*>(&evt.device_id),
				reinterpret_cast<uint8_t*>(&evt.signal_id),
				evt.logical_time,
				static_cast<float>(evt.device_time),
				static_cast<float>(evt.level),
				evt.parameters,
				evt.info);
		}

		// and pass it further
		mOutput->send(&evt);
	}
}

HRESULT CInterop_Export_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	// no configuration needs to be done

	Run_Main();

	return S_OK;
}

extern "C" void Register_Listener(TInterop_Listener listener)
{
	_registeredListener = listener;
}
