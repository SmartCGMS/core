#pragma once

#include "../../../common/iface/SolverIface.h"


extern "C" HRESULT IfaceCalling do_create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric);