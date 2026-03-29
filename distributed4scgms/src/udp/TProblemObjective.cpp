#include "TProblemObjective.h"

#include <numeric>
#include <algorithm>
#include <execution>


//#####################################################################################
//# Solver interface wrapper
//#####################################################################################

BOOL IfaceCalling Fitness_Wrapper(const void* data, const size_t solution_count, const double* solutions, double* const fitnesses) {
    CCommon_Problem *fitness = reinterpret_cast<CCommon_Problem*>(const_cast<void*>(data));

    const size_t problemSize = fitness->Problem_Size();

    std::vector<size_t> solidx(solution_count);
    std::iota(solidx.begin(), solidx.end(), 0);

    // TODO: REMOVE PARALLELISM?

    //for (size_t i = 0; i < solution_count; i++)
    std::for_each(std::execution::par_unseq, solidx.begin(), solidx.end(), [&fitnesses, &solutions, &fitness, problemSize](size_t i) {
        fitnesses[i] = fitness->Calculate_Fitness(&solutions[i * problemSize]);
    });

    return TRUE;
}

