#pragma once

#include "../../../common/iface/SolverIface.h"


extern "C" HRESULT IfaceCalling do_create_metric(const GUID *metric_id, const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric);