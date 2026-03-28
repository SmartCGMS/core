#pragma once

#pragma once

#include "TProblemData.h"


//#####################################################################################
//# Solver interface wrapper
//#####################################################################################

// Fitness_Wrapper bridges CCommon_Problem to the TSolver_Setup objective function interface.
// Pass a CCommon_Problem* as the `data` argument and this function as the `objective` argument
// of TSolver_Setup. It evaluates all solutions in parallel using std::execution::par_unseq.
BOOL IfaceCalling Fitness_Wrapper(const void* data, const size_t count, const double* solution, double* const fitnesss);