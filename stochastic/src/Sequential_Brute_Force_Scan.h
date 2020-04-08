#pragma once

#include <vector>
#include <mutex>
#include <shared_mutex>

#include "solution.h"
#include "../../../common/rtl/SolverLib.h"

template <typename TUsed_Solution>
class CSequential_Brute_Force_Scan {
protected:
	template <typename TSolution>
	struct TCandidate_Solution {
		TSolution solution;
		double fitness;
	};
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
	std::vector<TUsed_Solution> mStepping;
protected:
	solver::TSolver_Setup mSetup;
public:
	CSequential_Brute_Force_Scan(const solver::TSolver_Setup &setup) :
		mSetup(solver::Check_Default_Parameters(setup, /*100'000*/0, 100)),
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)) {

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
		for (auto i = 0; i < mSetup.population_size; i++) {
			step += stepping;
			mStepping.push_back(step);
		}
	}

	

	TUsed_Solution Solve(solver::TSolver_Progress &progress) {

		progress.current_progress = 0;
		progress.max_progress = mStepping.size()*mSetup.problem_size;


		TCandidate_Solution<TUsed_Solution> best_solution;
		best_solution.solution = mStepping[0];
		best_solution.fitness = mSetup.objective(mSetup.data, best_solution.solution.data());		
		progress.best_metric = best_solution.fitness;		

		std::atomic<size_t> atomic_progress{ 0 };
		std::shared_mutex best_mutex;

		std::vector<size_t> val_indexes(mStepping.size());
		std::iota(val_indexes.begin(), val_indexes.end(), 0);

		for (size_t param_idx = 0; param_idx < mSetup.problem_size; param_idx++) {			
			if (progress.cancelled != FALSE) break;

			if (mLower_Bound[param_idx] != mUpper_Bound[param_idx]) {													
								
				std::for_each(std::execution::par_unseq, val_indexes.begin(), val_indexes.end(),
					[this, &best_solution, &best_mutex, &progress, &atomic_progress, &param_idx](const auto &val_idx) {
					
					progress.current_progress = atomic_progress.fetch_add(1);
					if (progress.cancelled != FALSE) return;
				
					static thread_local TUsed_Solution local_solution;
					{
						std::shared_lock read_lock{ best_mutex };
						local_solution = best_solution.solution;
					}
					local_solution(param_idx) = mStepping[val_idx](param_idx);
					const double local_solution_fitness = mSetup.objective(mSetup.data, local_solution.data());

					if (local_solution_fitness < best_solution.fitness) {
						std::unique_lock write_lock{ best_mutex };

						best_solution.solution = local_solution;
						best_solution.fitness = local_solution_fitness;
						progress.best_metric = local_solution_fitness;
					}
				});
			}
		}

		return best_solution.solution;
	}
};