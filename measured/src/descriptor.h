#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"

extern "C" HRESULT IfaceCalling do_create_measured_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal);
