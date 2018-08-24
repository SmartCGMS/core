#pragma once


#include "LocalSearch.h"


template <typename TSolution, typename TFitness, bool _Preserve_Combination_Order>
class CRecursive_Combinatorial_Descent {
protected:
	const size_t mCombinatorial_Granularity = 10;
	const floattype mScale_Factor = 0.1;
protected:
	const TSolution mLower_Bound;
	const TSolution mUpper_Bound;
	TAligned_Solution_Vector<TSolution> mInitial_Solutions;
protected:
	TFitness &mFitness;
	SMetricFactory &mMetric_Factory;
public:
	CRecursive_Combinatorial_Descent(const TAligned_Solution_Vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, SMetricFactory &metric_factory) :
		mInitial_Solutions(initial_solutions), mLower_Bound(lower_bound), mUpper_Bound(upper_bound), mFitness(fitness), mMetric_Factory(metric_factory) {
	}


	TSolution Solve(volatile TSolverProgress &progress) {


		TSolution local_lower_bound = mLower_Bound;
		TSolution local_upper_bound = mUpper_Bound;
		
		size_t iter_count = 100;

		decltype(mInitial_Solutions) local_solutions = { mInitial_Solutions };

		TSolution previous_local_result = mInitial_Solutions[0];
		floattype previous_local_fitness = mFitness.Calculate_Fitness(previous_local_result, mMetric_Factory.CreateCalculator());

		progress.MaxProgress = 100;
		progress.CurrentProgress = 0;

		while (iter_count-- > 0) {
			CLocalSearch<TSolution, TFitness, _Preserve_Combination_Order, _Preserve_Combination_Order ? 20 : 100'000> local_search(local_solutions, local_lower_bound, local_upper_bound, mFitness, mMetric_Factory);
			TSolverProgress tmp_progress;
			TSolution present_local_result = local_search.Solve(tmp_progress);

			floattype present_local_fitness = mFitness.Calculate_Fitness(present_local_result, mMetric_Factory.CreateCalculator());
			if (present_local_fitness >= previous_local_fitness) break;

			previous_local_result = present_local_result;
			previous_local_fitness = present_local_fitness;


			local_lower_bound = mLower_Bound.max(present_local_result*(1.0 - (1.0 / static_cast<floattype>(mScale_Factor))));
			local_upper_bound = mUpper_Bound.min(present_local_result*(1.0 + (1.0 / static_cast<floattype>(mScale_Factor))));

			local_solutions.clear();
			local_solutions.push_back(present_local_result);

			progress.CurrentProgress++;
			progress.BestMetric = present_local_fitness;
		}

		return previous_local_result;
	}

};