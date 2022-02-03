#pragma once

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <climits>

#include "solution.h"
#include "../../../common/rtl/SolverLib.h"

template <typename TUsed_Solution>
class CSequential_Convex_Scan {
protected:
	struct THint_Bounds {
		TUsed_Solution lower, upper;
	};

	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
	TUsed_Solution mBest_Hint;
	std::vector<THint_Bounds> mHints;	
protected:
	solver::TSolver_Setup mSetup;

protected:	
	struct TCandidate_Solution {
		TUsed_Solution solution;
		std::array<double, solver::Maximum_Objectives_Count> fitness{ std::numeric_limits<double>::quiet_NaN() };		
	};


	TCandidate_Solution Find_Extreme(const size_t param_idx, const TUsed_Solution &best, const THint_Bounds& bounds) {

		TCandidate_Solution result;
		result.solution = best;


		TUsed_Solution effective_low = best;
		TUsed_Solution effective_high = best;

		double experimental_low = effective_low[param_idx] = bounds.lower[param_idx];
		double experimental_high = effective_high[param_idx] = bounds.upper[param_idx];


		std::array<double, solver::Maximum_Objectives_Count> lower_fitness{ std::numeric_limits<double>::max() };
		std::array<double, solver::Maximum_Objectives_Count> upper_fitness{ std::numeric_limits<double>::max() };

		size_t iter_counter = 0;
		while (iter_counter++ < mSetup.population_size) {

			//1. divide the interval to three parts with borders at x1 and x2
			const double diff = effective_high[param_idx] - effective_low[param_idx];
			experimental_low = effective_low[param_idx] + diff / 3.0;
			experimental_high = effective_low[param_idx] + diff * 2.0 / 3.0;

			//2. find the fitness at both borders
			result.solution[param_idx] = experimental_low;
			if (mSetup.objective(mSetup.data, result.solution.data(), lower_fitness.data()) != TRUE) {
				for (auto& elem : lower_fitness)
					elem = std::numeric_limits<double>::quiet_NaN();
			}

			result.solution[param_idx] = experimental_high;
			if (mSetup.objective(mSetup.data, result.solution.data(), upper_fitness.data()) != TRUE) {
				for (auto& elem : upper_fitness)
					elem = std::numeric_limits<double>::quiet_NaN();
			}
			
			//3. and adjust the borders
			if (Compare_Solutions(lower_fitness.data(), upper_fitness.data(), mSetup.objectives_count)) {
				//upper fitness is worse, make it the new border; the convex extreme should be at the left
				effective_high[param_idx] = experimental_high;
			}
			else if (Compare_Solutions(upper_fitness.data(), lower_fitness.data(), mSetup.objectives_count)) {
				//lower border is worse, make it a new border; the convex extreme should lay to the right
				effective_low[param_idx] = experimental_low;
			}
			else {
				//as this solver requires convex fitness function with a single extreme
				//equal fitness of both borders mean that the extreme is between them
				effective_high[param_idx] = experimental_high;
				effective_low[param_idx] = experimental_low;
			}

		}
		


		//return that contracted border that's closer to the extreme
		if (Compare_Solutions(lower_fitness.data(), upper_fitness.data(), mSetup.objectives_count)) {
			result.solution[param_idx] = experimental_low;
			result.fitness = lower_fitness;
		}
		else {
			result.solution[param_idx] = experimental_high;
			result.fitness = upper_fitness;
		}

		return result;
	}
public:
	CSequential_Convex_Scan(const solver::TSolver_Setup &setup) :
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
		mSetup(solver::Check_Default_Parameters(setup, /*100'000*/0, 100)) {


		//store suggested params as already contracted borders to 10% of the default borders to make local search
		for (size_t i = 0; i < mSetup.hint_count; i++) {

			TUsed_Solution hint = mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size)));//also ensure the bounds

			constexpr double region_length = 0.05;	//5% at each side
			constexpr double cut_off = 1.0 - region_length;

			mHints.push_back({ mLower_Bound * region_length + hint * cut_off, mUpper_Bound * cut_off + hint * region_length });
		}

		//global borders must go as the last one to prefer user-supplied hints 
		mHints.push_back({ mLower_Bound, mUpper_Bound });


		//eventually, let's find the best solution
		if (setup.hint_count > 0) {
			mBest_Hint = Vector_2_Solution<TUsed_Solution>(mSetup.hints[0], setup.problem_size);
			std::array<double, solver::Maximum_Objectives_Count> best_hint_fitness;
			if (mSetup.objective(mSetup.data, mBest_Hint.data(), best_hint_fitness.data()) == TRUE) {
				for (size_t i = 1; i < mSetup.hint_count; i++) {	//check if any other solution is better or not
					TUsed_Solution candidate = Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size);
					
					std::array<double, solver::Maximum_Objectives_Count> candidate_fitness;
					if (mSetup.objective(mSetup.data, candidate.data(), candidate_fitness.data()) == TRUE) {						

						if (Compare_Solutions(candidate_fitness.data(), best_hint_fitness.data(), mSetup.objectives_count))	{
							best_hint_fitness = candidate_fitness;
							mBest_Hint = candidate;
						}
					}
				}
			}
		}
		else
			mBest_Hint = 0.5 * (mHints[0].upper + mHints[0].lower);

	}



	TUsed_Solution Solve(solver::TSolver_Progress &progress) {

		TCandidate_Solution best_solution;

		best_solution.solution = mBest_Hint;			
		if (mSetup.objective(mSetup.data, best_solution.solution.data(), best_solution.fitness.data()) != TRUE)
			return mBest_Hint;

		progress.best_metric = best_solution.fitness[0];
				
		progress.max_progress = mSetup.problem_size;

		std::shared_mutex best_mutex;
		std::vector<size_t> val_indexes(mHints.size());
		std::iota(val_indexes.begin(), val_indexes.end(), 0);

		bool improved = true;		

		size_t improved_counter = 0;
		while (improved && (improved_counter++ < mSetup.max_generations)) {
			improved = false;
			progress.current_progress = 0;

			for (size_t param_idx = 0; param_idx < mSetup.problem_size; param_idx++) {
				progress.current_progress++;

				if (progress.cancelled != FALSE) break;

				if (mLower_Bound[param_idx] != mUpper_Bound[param_idx]) {
					
					std::for_each(std::execution::par_unseq, val_indexes.begin(), val_indexes.end(),
						[this, &best_solution, &best_mutex, &progress, &param_idx, &improved](const auto& val_idx) {

						const TCandidate_Solution local_solution = Find_Extreme(param_idx, best_solution.solution, mHints[val_idx]);

						if (Compare_Solutions(local_solution.fitness.data(), best_solution.fitness.data(), mSetup.objectives_count)) {

							std::unique_lock write_lock{ best_mutex };

							//do not be so rush! verify that this is still the better solution
							if (Compare_Solutions(local_solution.fitness.data(), best_solution.fitness.data(), mSetup.objectives_count)) {
								best_solution.solution = local_solution.solution;
								best_solution.fitness = local_solution.fitness;
								progress.best_metric = local_solution.fitness[0];

								improved = true;
							}
						}
					});
				}
			}
		}

		return best_solution.solution;
	}
};