/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8
 * 301 00, Pilsen
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include <vector>


#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#undef max

template <typename TUsed_Solution>
struct TLCCandidate_Solution {
	TUsed_Solution solution;
	double fitness;
};


constexpr size_t Suggested_Stepping_Granularity(const bool _Preserve_Combination_Order) {
	return _Preserve_Combination_Order ? 25 : 10'0000;	
	//empirically determined - could go two more orders higher but seems to have no real effect, just do not go below - begins to be unstable
}

template <typename TUsed_Solution, typename TFitness, bool _Preserve_Combination_Order, size_t _Stepping_Granularity = Suggested_Stepping_Granularity(_Preserve_Combination_Order)>
class CLocalSearch {
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
	TAligned_Solution_Vector<TUsed_Solution> mStepping;
protected:
	TFitness &mFitness;
	glucose::SMetric &mMetric;
public:
	CLocalSearch(const TAligned_Solution_Vector<TUsed_Solution> &initial_solutions, const TUsed_Solution &lower_bound, const TUsed_Solution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :

		mStepping(initial_solutions), mLower_Bound(lower_bound), mUpper_Bound(upper_bound), mFitness(fitness), mMetric(metric) {
		//keep the initial solutions as first as the Solve relies on it
		//mStepping.insert(mStepping.end(), initial_solutions.begin(), initial_solutions.end()); - copy constructor

		TUsed_Solution stepping;
		stepping.resize(mUpper_Bound.cols());
		for (auto i = 0; i < mUpper_Bound.cols(); i++) {
			stepping(i) = (mUpper_Bound(i) - mLower_Bound(i)) / static_cast<double>(_Stepping_Granularity);
		}

		TUsed_Solution step = mLower_Bound;
		mStepping.push_back(step);
		for (auto i = 0; i < _Stepping_Granularity; i++) {
			step += stepping;
			mStepping.push_back(step);
		}
	}

	TUsed_Solution Solve(volatile glucose::TSolver_Progress &progress) {

		const TLCCandidate_Solution<TUsed_Solution> init_solution { mStepping[0],
															   mFitness.Calculate_Fitness(init_solution.solution, mMetric) };

		const size_t steps_to_go = mStepping.size();
		
		progress.current_progress = 0;
		progress.max_progress = steps_to_go;
		progress.best_metric = init_solution.fitness;

		auto result = tbb::parallel_reduce(tbb::blocked_range<size_t>(size_t(0), steps_to_go),
			init_solution,

			[=, &progress](const tbb::blocked_range<size_t> &r, TLCCandidate_Solution<TUsed_Solution> other)->TLCCandidate_Solution<TUsed_Solution> {
				if (progress.cancelled != 0) return init_solution;

				const auto param_count = mLower_Bound.cols();
				glucose::SMetric metric_calculator = mMetric.Clone();

				TLCCandidate_Solution<TUsed_Solution> local_solution, best_solution;
				local_solution.solution.resize(param_count);
				best_solution.fitness = std::numeric_limits<double>::max();
				if (other.fitness < best_solution.fitness)
					best_solution = other;

				std::vector<size_t> index(param_count, 0);

				const auto r_end = r.end();
				for (auto iter = r.begin(); iter != r_end; ++iter) {
					index[0] = iter;

					if (_Preserve_Combination_Order) {
						bool tried_all_combinations = false;
						do {
							//evalute the metric
							for (auto j = 0; j < param_count; j++)
								local_solution.solution(j) = mStepping[index[j]](j);
							local_solution.fitness = mFitness.Calculate_Fitness(local_solution.solution, metric_calculator);
							if (local_solution.fitness < best_solution.fitness) {
								best_solution = local_solution;
							}

							//and move to the next
							index[param_count - 1]++; //first, increase the last digit and then run the carry algorithm
							for (auto carry_index = param_count - 1; carry_index != 0; carry_index--) {
								if (index[carry_index] >= steps_to_go) {
									index[carry_index] = 0;
									if (carry_index != 1) index[carry_index - 1]++;
									else tried_all_combinations = true;
								}
								else break;	//no carry, no need to continue
							}
						} while (!tried_all_combinations);
					} else {
						best_solution = init_solution;

						for (auto i = 0; i < param_count; i++) {
							local_solution.solution = best_solution.solution;
							local_solution.solution(i) = mStepping[iter](i);

								local_solution.fitness = mFitness.Calculate_Fitness(local_solution.solution, metric_calculator);
								if (local_solution.fitness < best_solution.fitness) {
									best_solution = local_solution;
								}
					
						}
					}	//if (_Preserve_Combination_Order)
				}

				progress.current_progress++;

				return best_solution;
			},

			[&progress](TLCCandidate_Solution<TUsed_Solution> a, TLCCandidate_Solution<TUsed_Solution> b)->TLCCandidate_Solution<TUsed_Solution> {
				if (a.fitness < b.fitness) {
					progress.best_metric = a.fitness;
					return a;
				}
				else {
					progress.best_metric = b.fitness;
					return b;
				}
			}
		);

		return result.solution;
	}
};
