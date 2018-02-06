#pragma once

#include <vector>


#include "solution.h"
#include "fitness.h"
#include "composite_fitness.h"

#include "../..\..\common\rtl\cfixes.h"

template <typename TSolution, typename TFitness>
class CNullMethod {
protected:
	TSolution mSolution;
public:
	CNullMethod(const std::vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, SMetricFactory &metric_factory) :
	mSolution(initial_solutions[0]) {};

	TSolution Solve(volatile TSolverProgress &progress) { return mSolution; };
};