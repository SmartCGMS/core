/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#pragma once

#include <vector>
#include <random>


#include "solution.h"
#include "fitness.h"
#include "nlopt.h"
#include "LocalSearch.h"


#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#undef max


template <typename TResult_Solution, typename TFitness, typename TTop_Solution = TResult_Solution, typename TBottom_Solution = TResult_Solution, typename TBottom_Solver = CNullMethod<TResult_Solution, CNLOpt_Fitness_Proxy<TFitness, TResult_Solution, TResult_Solution>>, size_t max_eval = 100>
class CRandom_Search {
protected:
	TBottom_Solution mLower_Bottom_Bound;
	TBottom_Solution mUpper_Bottom_Bound;
	TResult_Solution mInitial_Solution;
	TTop_Solution mLower_Top_Bound;
	TTop_Solution mRange_Top_Bound;
protected:
	std::random_device mRandom_Device;
	std::mt19937 mRandom_Generator{ mRandom_Device() };
	std::uniform_real_distribution<floattype> mUniform_Distribution{ 0.0, 1.0 };
protected:
	TFitness &mFitness;
	SMetricFactory &mMetric_Factory;
public:
	CRandom_Search(const TAligned_Solution_Vector<TResult_Solution> &initial_solutions, const TResult_Solution &lower_bound, const TResult_Solution &upper_bound, TFitness &fitness, SMetricFactory &metric_factory) :

		mFitness(fitness), mMetric_Factory(metric_factory) {

		if (initial_solutions.size() > 0) mInitial_Solution = initial_solutions[0];

		TTop_Solution upper_top_bound;
		mLower_Bottom_Bound = mLower_Top_Bound.Decompose(lower_bound);
		mUpper_Bottom_Bound = upper_top_bound.Decompose(upper_bound);


		mRange_Top_Bound = upper_top_bound - mLower_Top_Bound;
	}

	

	TResult_Solution Solve(volatile TSolverProgress &progress) {

		const TLCCandidate_Solution<TResult_Solution> init_solution { mInitial_Solution,
																 mFitness.Calculate_Fitness(init_solution.solution, mMetric_Factory.CreateCalculator()) };

		
		progress.CurrentIteration = 0;
		progress.MaxIterations = max_eval;
		progress.BestMetric = init_solution.fitness;

		auto result = tbb::parallel_reduce(tbb::blocked_range<size_t>(size_t(0), max_eval),
			init_solution,

			[=, &progress](const tbb::blocked_range<size_t> &r, const TLCCandidate_Solution<TResult_Solution> &other)->TLCCandidate_Solution<TResult_Solution> {
				if (progress.Cancelled != 0) return other;

				SMetricCalculator metric_calculator = mMetric_Factory.CreateCalculator();
				TLCCandidate_Solution<TResult_Solution> best_solution;

				auto Evaluate_Top_Solution = [&](const auto &top)->TLCCandidate_Solution<TResult_Solution> {
					CNLOpt_Fitness_Proxy<TFitness, TTop_Solution, TBottom_Solution> fitness_proxy{ mFitness, top };
					const TAligned_Solution_Vector<TBottom_Solution> bottom_init;
					TBottom_Solver bottom_solver(bottom_init, mLower_Bottom_Bound, mUpper_Bottom_Bound, fitness_proxy, mMetric_Factory);					
					TBottom_Solution bottom_result = bottom_solver.Solve(progress);

					TLCCandidate_Solution<TResult_Solution> result;
					result.solution = top.Compose(bottom_result);
					result.fitness = mFitness.Calculate_Fitness(result.solution, metric_calculator);

					return result;
				};

				
				auto Strategy_1 = [&](const TResult_Solution &best)->TLCCandidate_Solution<TResult_Solution> {
					//Strategy 1 - let's generate completely new tree and evaluate it			
					TTop_Solution top_result;
					top_result.Decompose(best);


					const floattype mutation_threshold = mUniform_Distribution(mRandom_Generator);
					for (auto j = 0; j < top_result.cols(); j++)
						if (mutation_threshold > mUniform_Distribution(mRandom_Generator))
							top_result(j) = mRange_Top_Bound(j)*mUniform_Distribution(mRandom_Generator) + mLower_Top_Bound(j);

					return Evaluate_Top_Solution(top_result);
				};
				
				auto Strategy_2 = [&](const TResult_Solution &best)->TLCCandidate_Solution<TResult_Solution> {
					//Strategy 2 - let's mutate the tree node by node and save beneficial changes only
					TTop_Solution top_result;
					top_result.Decompose(best);

					TLCCandidate_Solution<TResult_Solution> best_local_solution{ best, mFitness.Calculate_Fitness(best, metric_calculator) };

					for (auto j = 0; j < top_result.cols(); j++) {
						if (progress.Cancelled != 0) break;

						if (mRange_Top_Bound(j) > 0.0) {
							floattype old_value = top_result(j);
							top_result(j) = mRange_Top_Bound(j)*mUniform_Distribution(mRandom_Generator) + mLower_Top_Bound(j);
							
							TLCCandidate_Solution<TResult_Solution> local_solution = Evaluate_Top_Solution(top_result);
							if (local_solution.fitness < best_local_solution.fitness) best_local_solution = local_solution;	//succeeded
								else top_result(j) = old_value;		//no success
						}
					}

					return best_local_solution;
				};

				best_solution.fitness = std::numeric_limits<floattype>::max();
				if (other.fitness < best_solution.fitness)
					best_solution = other;


				const auto r_end = r.end();
				for (auto iter = r.begin(); iter != r_end; ++iter) {

					//there two possible strategies
					auto local_solution = mUniform_Distribution(mRandom_Generator) > 0.5 ? Strategy_1(best_solution.solution) : Strategy_2(best_solution.solution);

					if (local_solution.fitness < best_solution.fitness) {
						best_solution = local_solution;
						progress.BestMetric = best_solution.fitness;
					}								

					progress.CurrentIteration++;
				}

				return best_solution;
		},

			[&progress](const TLCCandidate_Solution<TResult_Solution> &a, const TLCCandidate_Solution<TResult_Solution> &b)->TLCCandidate_Solution<TResult_Solution> {
				if (a.fitness < b.fitness) {
					progress.BestMetric = a.fitness;
					return a;
				}
				else {
					progress.BestMetric = b.fitness;
					return b;
				}
		}
		);

		return result.solution;
	}
};