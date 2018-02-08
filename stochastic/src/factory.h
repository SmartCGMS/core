#pragma once

#include "../../../common/iface/SolverIface.h"


extern "C" HRESULT IfaceCallingdo_solve_model_parameters(const GUID *solver_id, const GUID *signal_id, const glucose::ITime_Segment **segments, const size_t segment_count, glucose::IMetric *metric,
														glucose::IModel_Parameter_Vector *lower_bound, glucose::IModel_Parameter_Vector *upper_bound, glucose::IModel_Parameter_Vector **solved_parameters);