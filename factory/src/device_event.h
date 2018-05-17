#pragma once

#include "../../../common/iface/DeviceIface.h"


class CDevice_Event : public virtual glucose::IDevice_Event {
protected:
	glucose::TDevice_Event mRaw;
public:
	CDevice_Event(glucose::NDevice_Event_Code code);
	virtual ~CDevice_Event();
	virtual ULONG IfaceCalling Release() override;					
	virtual HRESULT IfaceCalling Raw(glucose::TDevice_Event **dst) override;
};

#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_device_event(glucose::NDevice_Event_Code code, glucose::IDevice_Event **event);
#else
	extern "C" HRESULT IfaceCalling create_device_event(glucose::NDevice_Event_Code code, glucose::IDevice_Event **event);
#endif
