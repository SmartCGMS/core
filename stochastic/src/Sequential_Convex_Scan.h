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
	using TFitness_Comparator_Proc = bool(CSequential_Convex_Scan<TUsed_Solution>::*)(const solver::TFitness&, const solver::TFitness&) const;
	TFitness_Comparator_Proc mFitness_Comparator = &CSequential_Convex_Scan<TUsed_Solution>::Compare_Solutions_Default;

	const NFitness_Strategy mFitness_Strategy = NFitness_Strategy::Master;
	bool Compare_Solutions_Default(const solver::TFitness& better, const solver::TFitness& worse) const {
		return Compare_Solutions(better, worse, mSetup.objectives_count, mFitness_Strategy);
	}

	bool Compare_Solutions_User(const solver::TFitness& better, const solver::TFitness& worse) const {
		return mSetup.comparator(better.data(), worse.data(), mSetup.objectives_count);			
	}
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
		solver::TFitness fitness = solver::Nan_Fitness;
	};


	TCandidate_Solution Find_Extreme(const size_t param_idx, const TUsed_Solution &best, const THint_Bounds& bounds) {

		TCandidate_Solution result;
		result.solution = best;
		result.fitness = solver::Nan_Fitness;

		std::vector<double> candidate(2 * mSetup.problem_size);		
		std::array<solver::TFitness, 2> candidate_fitness{ solver::Nan_Fitness, solver::Nan_Fitness };

		{
			const size_t block_size = mSetup.problem_size * sizeof(double);
			memcpy(candidate.data(), best.data(), block_size);
			memcpy(candidate.data() + mSetup.problem_size, best.data(), block_size);
		}


		double effective_low = bounds.lower[param_idx], experimental_low = bounds.lower[param_idx];
		double effective_high = bounds.upper[param_idx],  experimental_high = bounds.upper[param_idx];

		constexpr size_t low_idx = 0;
		constexpr size_t high_idx = 1;

		size_t iter_counter = 0;
		while (iter_counter++ < mSetup.population_size) {

			//1. divide the interval to three parts with borders at x1 and x2
			const double diff = effective_high - effective_low;
			experimental_low = effective_low + diff / 3.0;
			experimental_high = effective_low + diff * 2.0 / 3.0;

			candidate[param_idx] = experimental_low;
			candidate[mSetup.problem_size + param_idx] = experimental_high;

			//2. find the fitness at both borders
			if (mSetup.objective(mSetup.data, 2, candidate.data(), reinterpret_cast<double*>(candidate_fitness.data())) != TRUE)
				break;

			//3. and adjust the borders
			if ((*this.*mFitness_Comparator)(candidate_fitness[low_idx], candidate_fitness[high_idx])) {
				//upper fitness is worse, make it the new border; the convex extreme should be at the left
				effective_high = experimental_high;
			} else
				if ((*this.*mFitness_Comparator)(candidate_fitness[high_idx], candidate_fitness[low_idx])) {
					//lower border is worse, make it a new border; the convex extreme should lay to the right
					effective_low = experimental_low;
				} else {
					//as this solver requires convex fitness function with a single extreme
					//equal fitness of both borders mean that the extreme is between them
					effective_high = experimental_high;
					effective_low = experimental_low;
				}
		}
		


		//return that contracted border that's closer to the extreme
		if ((*this.*mFitness_Comparator)(candidate_fitness[low_idx], candidate_fitness[high_idx])) {
			result.solution[param_idx] = experimental_low;
			result.fitness = candidate_fitness[low_idx];
		}
		else {
			result.solution[param_idx] = experimental_high;
			result.fitness = candidate_fitness[high_idx];
		}

		return result;
	}
public:
	CSequential_Convex_Scan(const solver::TSolver_Setup &setup) :
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
		mSetup(solver::Check_Default_Parameters(setup, /*100'000*/0, 100)) {


		//setup custom comparator, before we use it
		if (mSetup.comparator)
			mFitness_Comparator = &CSequential_Convex_Scan<TUsed_Solution>::Compare_Solutions_User;


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
			solver::TFitness best_hint_fitness{solver::Nan_Fitness};
			if (mSetup.objective(mSetup.data, 1, mBest_Hint.data(), best_hint_fitness.data()) == TRUE) {
				for (size_t i = 1; i < mSetup.hint_count; i++) {	//check if any other solution is better or not
					TUsed_Solution candidate = Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size);
					
					solver::TFitness candidate_fitness{ solver::Nan_Fitness };
					if (mSetup.objective(mSetup.data, 1, candidate.data(), candidate_fitness.data()) == TRUE) {						

						if ((*this.*mFitness_Comparator)(candidate_fitness, best_hint_fitness))	{
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

		progress = solver::Null_Solver_Progress;

		TCandidate_Solution best_solution;		

		best_solution.solution = mBest_Hint;			
		best_solution.fitness = solver::Nan_Fitness;
		if (mSetup.objective(mSetup.data, 1, best_solution.solution.data(), best_solution.fitness.data()) != TRUE)
			return mBest_Hint;

		progress.best_metric = best_solution.fitness;
				
		progress.max_progress = mSetup.problem_size;

		std::shared_mutex best_mutex;
		std::vector<size_t> val_indexes(mHints.size());
		std::iota(val_indexes.begin(), val_indexes.end(), 0);

		bool improved = true;		

		size_t improved_counter = 0;
		while (improved && (improved_counter++ < mSetup.max_generations)) {
			improved = false;
			progress.current_progress = 0;

			for (size_t param_idx_offset = 0; param_idx_offset < mSetup.problem_size; param_idx_offset++) {
				progress.current_progress++;

				if (progress.cancelled != FALSE) break;

				const size_t effective_param_idx = mSetup.problem_size - param_idx_offset - 1;	//let's prefer the segment common parameters, which are stored as last 

				if (mLower_Bound[effective_param_idx] != mUpper_Bound[effective_param_idx]) {
					
					std::for_each(std::execution::par, val_indexes.begin(), val_indexes.end(),
						[this, &best_solution, &best_mutex, &progress, &effective_param_idx, &improved](const auto& val_idx) {

						const TCandidate_Solution local_solution = Find_Extreme(effective_param_idx, best_solution.solution, mHints[val_idx]);

						if ((*this.*mFitness_Comparator)(local_solution.fitness, best_solution.fitness)) {

							std::unique_lock write_lock{ best_mutex };

							//do not be so rush! verify that this is still the better solution
							if ((*this.*mFitness_Comparator)(local_solution.fitness, best_solution.fitness)) {
								best_solution.solution = local_solution.solution;
								best_solution.fitness = local_solution.fitness;
								progress.best_metric = local_solution.fitness;

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