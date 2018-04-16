#include "device.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/rattime.h"

#include <set>
#include <chrono>

// set of all device filters
static std::set<CDevice_Filter*> device_filters;

CDevice_Filter::CDevice_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
	: mInput(inpipe), mOutput(outpipe), mLogicalTime(0)
{
	device_filters.insert(this);
}

CDevice_Filter::~CDevice_Filter()
{
	device_filters.erase(this);
}

HRESULT CDevice_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	// no configuration needs to be done (yet)

	// TODO: configurable device GUID (from BLE/other address)

	return S_OK;
}

void CDevice_Filter::Signal_CGMS(float value)
{
	glucose::TDevice_Event evt;

	evt.event_code = glucose::NDevice_Event_Code::Level;
	evt.device_id = { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF } }; // TODO: real value, see CDevice_Filter::configure
	evt.signal_id = glucose::signal_IG;
	evt.level = value;

	// since this is considered "device" filter, the timestamp is built here - current time is considered the current device time, from which the value originated

	time_t t_now = static_cast<time_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

	evt.device_time = Unix_Time_To_Rat_Time(t_now);
	evt.logical_time = mLogicalTime;

	mLogicalTime++;

	mOutput->send(&evt);
}

extern "C" void signal_cgms(float value)
{
	// notify all device filters about the incoming value
	for (auto& filter : device_filters)
		filter->Signal_CGMS(value);
}
