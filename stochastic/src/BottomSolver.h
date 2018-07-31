#pragma once

#include <vector>
#include <random>


#include "solution.h"
#include "fitness.h"
#include "composite_fitness.h"
#include "nlopt.h"
#include "LocalSearch.h"

//#include "../..\..\common\rtl\cfixes.h"

#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#undef max


template <typename TResult_Solution, typename TFitness, typename TTop_Solution = TResult_Solution, typename TBottom_Solution = TResult_Solution, typename TBottom_Solver = CNullMethod<TResult_Solution, CNLOpt_Fitness_Proxy<TFitness, TResult_Solution, TResult_Solution>>>
class CBottom_Solver {
protected:
	TBottom_Solution mLower_Bottom_Bound;
	TBottom_Solution mUpper_Bottom_Bound;
	TResult_Solution mInitial_Solution;
protected:
	TFitness &mFitness;
	SMetricFactory &mMetric_Factory;
public:
	CBottom_Solver(const std::vector<TResult_Solution> &initial_solutions, const TResult_Solution &lower_bound, const TResult_Solution &upper_bound, TFitness &fitness, SMetricFactory &metric_factory) :

		mFitness(fitness), mMetric_Factory(metric_factory) {

		if (initial_solutions.size() > 0) mInitial_Solution = initial_solutions[0];

		TTop_Solution lower_top_bound, upper_top_bound;
		mLower_Bottom_Bound = lower_top_bound.Decompose(lower_bound);
		mUpper_Bottom_Bound = upper_top_bound.Decompose(upper_bound);
	}

	

	TResult_Solution Solve(volatile TSolverProgress &progress) {

		TTop_Solution top_result;
		const std::vector<TBottom_Solution> bottom_init = { top_result.Decompose(mInitial_Solution) };	//also initializes top_results
		CNLOpt_Fitness_Proxy<TFitness, TTop_Solution, TBottom_Solution> fitness_proxy{ mFitness, top_result }; //that's needed by fitness_proxy
		TBottom_Solver bottom_solver(bottom_init, mLower_Bottom_Bound, mUpper_Bottom_Bound, fitness_proxy, mMetric_Factory);
		TBottom_Solution bottom_result = bottom_solver.Solve(progress);

		return top_result.Compose(bottom_result);
	}
};