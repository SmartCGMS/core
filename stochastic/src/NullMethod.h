#pragma once

#include <vector>


#include "solution.h"
#include "fitness.h"

template <typename TSolution, typename TFitness>
class CNullMethod {
protected:
	TSolution mSolution;
public:
	CNullMethod(const std::vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :
		mSolution(initial_solutions[0]) {};

	TSolution Solve(volatile glucose::TSolver_Progress &progress) { return mSolution; };
};