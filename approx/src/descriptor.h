#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/iface/ApproxIface.h"
#include "../../../common/rtl/hresult.h"

extern "C" HRESULT IfaceCalling do_get_approximator_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApproximator **approx, glucose::IApprox_Parameters_Vector* configuration);
