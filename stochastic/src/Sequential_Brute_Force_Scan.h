#pragma once

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <climits>

#include "solution.h"
#include "../../../common/rtl/SolverLib.h"

template <typename TUsed_Solution>
class CSequential_Brute_Force_Scan {
protected:
	const NFitness_Strategy mFitness_Strategy = NFitness_Strategy::Master;
protected:
	template <typename TSolution>
	struct TCandidate_Solution {
		TSolution solution;
		solver::TFitness fitness = solver::Nan_Fitness;
	};
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
	std::vector<TUsed_Solution> mStepping;
protected:
	solver::TSolver_Setup mSetup;
public:
	CSequential_Brute_Force_Scan(const solver::TSolver_Setup &setup) :
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
		mSetup(solver::Check_Default_Parameters(setup, /*100'000*/0, 100)) {

		//a) by storing suggested params
		for (size_t i = 0; i < mSetup.hint_count; i++) {	
			mStepping.push_back(mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size))));//also ensure the bounds
		}

		TUsed_Solution stepping;
		stepping.resize(mUpper_Bound.cols());
		for (auto i = 0; i < mUpper_Bound.cols(); i++) {
			stepping(i) = (mUpper_Bound(i) - mLower_Bound(i)) / static_cast<double>(mSetup.population_size);
		}

		TUsed_Solution step = mLower_Bound;
		mStepping.push_back(step);
		for (size_t i = 0; i < mSetup.population_size; i++) {
			step += stepping;
			mStepping.push_back(step);
		}
	}

	

	TUsed_Solution Solve(solver::TSolver_Progress &progress) {

		progress = solver::Null_Solver_Progress;

		progress.current_progress = 0;
		progress.max_progress = mStepping.size()*mSetup.problem_size;


		TCandidate_Solution<TUsed_Solution> best_solution;
		best_solution.solution = mStepping[0];
		best_solution.fitness = solver::Nan_Fitness;

		if (mSetup.objective(mSetup.data, best_solution.solution.data(), best_solution.fitness.data()) != TRUE)
			return best_solution.solution;			//TODO: this is not necessarily the best solution, just the first one!

		progress.best_metric = best_solution.fitness;

		
		std::atomic<size_t> atomic_progress{ 0 };
		std::shared_mutex best_mutex;

		std::vector<size_t> val_indexes(mStepping.size());
		std::iota(val_indexes.begin(), val_indexes.end(), 0);

		bool improved = true;

		size_t improved_counter = 0;
		while (improved && (improved_counter++ < mSetup.max_generations)) {
			improved = false;
			progress.current_progress = 0;
			atomic_progress.store(0);	//running next generation

			for (size_t param_idx = 0; param_idx < mSetup.problem_size; param_idx++) {
				if (progress.cancelled != FALSE) break;

				if (mLower_Bound[param_idx] != mUpper_Bound[param_idx]) {

					std::for_each(std::execution::par_unseq, val_indexes.begin(), val_indexes.end(),
						[this, &best_solution, &best_mutex, &progress, &atomic_progress, &param_idx, &improved](const auto &val_idx) {

						progress.current_progress = atomic_progress.fetch_add(1);
						if (progress.cancelled != FALSE) return;

						static thread_local TUsed_Solution local_solution;
						static thread_local solver::TFitness  local_solution_fitness = solver::Nan_Fitness;
						{
							std::shared_lock read_lock{ best_mutex };
							local_solution = best_solution.solution;
						}
						local_solution(param_idx) = mStepping[val_idx](param_idx);
						if (mSetup.objective(mSetup.data, local_solution.data(), local_solution_fitness.data()) == TRUE) {
							
							if (Compare_Solutions(local_solution_fitness, best_solution.fitness, mSetup.objectives_count, mFitness_Strategy)) {
								std::unique_lock write_lock{ best_mutex };

								//do not be so rush! verify that this is still the better solution
								if (Compare_Solutions(local_solution_fitness, best_solution.fitness, mSetup.objectives_count, mFitness_Strategy)) {
									best_solution.solution = local_solution;
									best_solution.fitness = local_solution_fitness;
									progress.best_metric = local_solution_fitness;

									improved = true;
								}
							}
						}
					});
				}
			}
		}

		return best_solution.solution;
	}
};