#pragma once

#include "../../../common/iface/SolverIface.h"


extern "C" HRESULT IfaceCalling do_create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal);