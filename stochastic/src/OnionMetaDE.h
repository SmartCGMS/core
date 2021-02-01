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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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
#include <random>
#include <algorithm>
#include <numeric>
#include <map>
#include <ctime>
#include <chrono>
#include <set>

#ifdef __cpp_lib_execution
	#include <execution>
#else
	namespace std
	{
		// minimal substitute for missing execution header (for pre C++20 compilers)

		enum class execution {
			par_unseq
		};

		template<typename Fnc, typename It>
		void for_each(const execution ex, const It& begin, const It& end, Fnc fnc)
		{
			for (It itr = begin; itr != end; itr++)
				fnc(*itr);
		}
	}
#endif

#undef max
#undef min

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/utils/DebugHelper.h"

namespace onion_metade
{
	enum class NStrategy : size_t { desCurrentToPBest = 0, desCurrentToUmPBest, desBest2Bin, desUmBest1, desCurrentToRand1, desTournament, count };

	template <typename TUsed_Solution>
	struct TOnionMetaDE_Candidate_Solution {
		size_t population_index = 0;	//only because of the tournament and =0 to keep static analyzer happy
		NStrategy strategy = NStrategy::desCurrentToPBest;
		double CR = 0.5, F = 1.0;
		TUsed_Solution current, next;
		double current_fitness = std::numeric_limits<double>::max();
		double next_fitness = std::numeric_limits<double>::max();
	};
}


template <typename TUsed_Solution, typename TRandom_Device = std::random_device>
class COnionMetaDE
{
	protected:
		static constexpr size_t mPBest_Count = 5;
		const double mCR_min = 0.0;
		const double mCR_range = 1.0;
		const double mF_min = 0.0;
		const double mF_range = 2.0;

	protected:
		const solver::TSolver_Setup mSetup;
		TUsed_Solution mLower_Bound;
		TUsed_Solution mUpper_Bound;
		TAligned_Solution_Vector<onion_metade::TOnionMetaDE_Candidate_Solution<TUsed_Solution>> mPopulation;
		std::vector<size_t> mPopulation_Best;
		void Generate_Meta_Params(onion_metade::TOnionMetaDE_Candidate_Solution<TUsed_Solution> &solution) {
			solution.CR = mCR_min + mCR_range*mUniform_Distribution_dbl(mRandom_Generator);
			solution.F = mF_min + mF_range*mUniform_Distribution_dbl(mRandom_Generator);

			const auto old_strategy = solution.strategy;
			decltype(solution.strategy) new_strategy;
			do {
				new_strategy = static_cast<onion_metade::NStrategy>(mUniform_Distribution_Strategy(mRandom_Generator));
			} while (old_strategy == new_strategy);

			solution.strategy = new_strategy;
		}

		inline const auto Select_Best_Candidate() const {
			return std::min_element(
				mPopulation.begin(),
				mPopulation.end(),
				[&](const onion_metade::TOnionMetaDE_Candidate_Solution<TUsed_Solution>& a, const onion_metade::TOnionMetaDE_Candidate_Solution<TUsed_Solution>& b) {
					return a.current_fitness < b.current_fitness;
				}
			);
		}

		inline void Generate_Random_Candidate(size_t idx, const TUsed_Solution& bounds_range) {
			TUsed_Solution tmp;

			// this helps when we use generic solution vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
			// optimized away in compile time
			tmp.resize(Eigen::NoChange, mSetup.problem_size);

			for (size_t j = 0; j < mSetup.problem_size; j++)
				tmp[j] = mUniform_Distribution_dbl(mRandom_Generator);

			mPopulation[idx].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
		}

	protected:
		inline static TRandom_Device mRandom_Generator;
		inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };
		inline static thread_local std::uniform_int_distribution<size_t> mUniform_Distribution_PBest{ 0, mPBest_Count-1 };
		inline static thread_local std::uniform_int_distribution<size_t> mUniform_Distribution_Strategy{ 0, static_cast<size_t>(onion_metade::NStrategy::count)-1 };

	public:
		COnionMetaDE(const solver::TSolver_Setup &setup) : 
			mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
			mSetup(solver::Check_Default_Parameters(setup, 100'000, 100))
		{
			mPopulation.resize(std::max(mSetup.population_size, mPBest_Count));
			mPopulation_Best.resize(mPopulation.size());

			//1. create the initial population
			const size_t initialized_count = std::min(mPopulation.size() / 2, mSetup.hint_count);

			//a) by storing suggested params
			for (size_t i = 0; i < initialized_count; i++) {
				mPopulation[i].current = mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size)));//also ensure the bounds
			}

			//b) by complementing it with randomly generated numbers
			const auto bounds_range = mUpper_Bound - mLower_Bound;
			for (size_t i = initialized_count; i < mPopulation.size(); i++) {
				Generate_Random_Candidate(i, bounds_range);
			}

			//c) and shuffle
			std::shuffle(mPopulation.begin(), mPopulation.end(), mRandom_Generator);

			//2. set cr and f parameters, and calculate the metrics		
			for (size_t i = 0; i < mPopulation.size(); i++) {
				auto &solution = mPopulation[i];
				Generate_Meta_Params(solution);
				solution.population_index = i;
			}

			std::for_each(std::execution::par_unseq, mPopulation.begin(), mPopulation.end(), [this](auto &candidate_solution) {
				candidate_solution.current_fitness = mSetup.objective(mSetup.data, candidate_solution.current.data());
			});

			//3. finally, create and fill mpopulation indexes
			std::iota(mPopulation_Best.begin(), mPopulation_Best.end(), 0);
		}


		TUsed_Solution Solve(solver::TSolver_Progress &progress) {

			progress.current_progress = 0;
			progress.max_progress = mSetup.max_generations;
	
			const size_t solution_size = mPopulation[0].current.cols();
			std::uniform_int_distribution<size_t> mUniform_Distribution_Solution{ 0, solution_size - 1 };	//this distribution must be local as solution can be dynamic
			std::uniform_int_distribution<size_t> mUniform_Distribution_Population{ 0, mPopulation.size() - 1 };

			const size_t onionRingCount = 50;
			const double onionRingRatio = 0.02; // subdivision ratio
			const double onionRingOverlapRatio = 0.1; // allow maximum of this % outer overlap ("return back" if the parameter is in % boundary range)
			const size_t onionGenStep = mSetup.max_generations / onionRingCount; // onionRingCount-1 subdivisions
			size_t currentRing = 0;

			while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == FALSE))
			{
				// peel the onion
				if (progress.current_progress / onionGenStep != currentRing)
				{
					currentRing = progress.current_progress / onionGenStep;

					const auto best = Select_Best_Candidate();

					const TUsed_Solution ldiff = best->current - mLower_Bound;
					const TUsed_Solution udiff = mUpper_Bound - best->current;
					const TUsed_Solution bdiff = mUpper_Bound - mLower_Bound;

					const auto lfactor = (ldiff / bdiff) * (1.0 + onionRingOverlapRatio) - onionRingOverlapRatio;
					const auto ufactor = (udiff / bdiff) * (1.0 + onionRingOverlapRatio) - onionRingOverlapRatio;

					mLower_Bound += onionRingRatio * lfactor * bdiff;
					mUpper_Bound -= onionRingRatio * ufactor * bdiff;

					const TUsed_Solution newbdiff = mUpper_Bound - mLower_Bound;
					std::for_each(std::execution::par_unseq, mPopulation_Best.begin(), mPopulation_Best.end(), [=](size_t i) {
						auto& candidate_solution = mPopulation[i];

						// TODO: more strategies for cutting candidate solutions, that are outside bounds

						if (!(candidate_solution.current > mLower_Bound).all() || !(candidate_solution.current < mUpper_Bound).all())
						{
							Generate_Random_Candidate(i, newbdiff);
							candidate_solution.current_fitness = mSetup.objective(mSetup.data, candidate_solution.current.data());
						}
					});
				}

				//1. determine the best p-count parameters, without actually re-ordering the population
				//we want to avoid of getting all params close together and likely loosing the population diversity
				std::partial_sort(mPopulation_Best.begin(), mPopulation_Best.begin() + mPBest_Count, mPopulation_Best.end(),
					[&](const size_t &a, const size_t &b) {return mPopulation[a].current_fitness < mPopulation[b].current_fitness; });

				//update the progress
				progress.best_metric = mPopulation[mPopulation_Best[0]].current_fitness;

				std::for_each(std::execution::par_unseq, mPopulation.begin(), mPopulation.end(), [=, &mUniform_Distribution_Solution, &mUniform_Distribution_Population](auto &candidate_solution) {

					auto random_difference_vector = [&]()->TUsed_Solution {
						const size_t idx1 = mUniform_Distribution_Population(mRandom_Generator);
						const size_t idx2 = mUniform_Distribution_Population(mRandom_Generator);
						return mPopulation[idx1].current - mPopulation[idx2].current;
					};

					switch (candidate_solution.strategy)
					{
						case onion_metade::NStrategy::desCurrentToPBest:
						{
							const size_t p_index = mUniform_Distribution_PBest(mRandom_Generator);
							candidate_solution.next = candidate_solution.current +
														candidate_solution.F*(mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
														candidate_solution.F*random_difference_vector();

							break;
						}
						case onion_metade::NStrategy::desCurrentToUmPBest:
						{
							const size_t p_index = mUniform_Distribution_PBest(mRandom_Generator);
							candidate_solution.next = candidate_solution.current +
														candidate_solution.F*(mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
														mUniform_Distribution_dbl(mRandom_Generator)*random_difference_vector();

							break;
						}
						case onion_metade::NStrategy::desBest2Bin:
						{
							candidate_solution.next = candidate_solution.current +
														candidate_solution.F * random_difference_vector() +
														candidate_solution.F * random_difference_vector();

							break;
						}
						case onion_metade::NStrategy::desUmBest1:
						{
							candidate_solution.next = candidate_solution.current +
														candidate_solution.F * (mPopulation[mPopulation_Best[0]].current - candidate_solution.current) +
														mUniform_Distribution_dbl(mRandom_Generator) * random_difference_vector();

							break;
						}
						case onion_metade::NStrategy::desCurrentToRand1:
						{
							candidate_solution.next = candidate_solution.current +
														mUniform_Distribution_dbl(mRandom_Generator) * random_difference_vector() +
														candidate_solution.F * random_difference_vector();
							break;
						}
						case onion_metade::NStrategy::desTournament:
						{
							//we choose mPBest_Count-number of random candidates for a direct crossbreeding with the current candidate solution

							size_t to_go = mPBest_Count;
							size_t best_tournament_index = candidate_solution.population_index;
							double best_tournament_fitness = std::numeric_limits<double>::max();
							std::set<size_t> visited_tournament_indexes{ best_tournament_index };
							while (to_go-- > 0) {
								size_t random_tournament_index = mUniform_Distribution_Population(mRandom_Generator);
								if (visited_tournament_indexes.find(random_tournament_index) == visited_tournament_indexes.end()) {
									visited_tournament_indexes.insert(random_tournament_index);

									if (mPopulation[random_tournament_index].current_fitness < best_tournament_fitness) {
										best_tournament_fitness = mPopulation[random_tournament_index].current_fitness;
										best_tournament_index = random_tournament_index;
									}
								}
							}

							candidate_solution.next = mPopulation[best_tournament_index].current;
							break;
						}
						default:
							break;
					}

					// ensure the bounds
					candidate_solution.next = mUpper_Bound.min(mLower_Bound.max(candidate_solution.next));

					// crossbreed
					{
						for (size_t element_iter = 0; element_iter < solution_size; element_iter++) {
							if (mUniform_Distribution_dbl(mRandom_Generator) > candidate_solution.CR)
								candidate_solution.next[element_iter] = candidate_solution.current[element_iter];
						}

						// and, we alway have to keep at least one original/current element
						const size_t element_to_replace = mUniform_Distribution_Solution(mRandom_Generator);
						candidate_solution.next[element_to_replace] = candidate_solution.current[element_to_replace];
					}

					// and evaluate
					candidate_solution.next_fitness = mSetup.objective(mSetup.data, candidate_solution.next.data());
				
				});


				// 3. Let us preserve the better vectors - too fast to amortize parallelization => serial code
				for (auto &solution : mPopulation)
				{
					// used strategy produced a better offspring => leave meta params as they are
					if (solution.current_fitness > solution.next_fitness)
					{
						solution.current = solution.next;
						solution.current_fitness = solution.next_fitness;
					}
					else
					{
						// the offspring is worse than its parents => modify parents' DE parameters
						Generate_Meta_Params(solution);
					}
				}
			}

			// find the best result and return it
			const auto result = std::min_element(
				mPopulation.begin(),
				mPopulation.end(),
				[&](const onion_metade::TOnionMetaDE_Candidate_Solution<TUsed_Solution> &a, const onion_metade::TOnionMetaDE_Candidate_Solution<TUsed_Solution> &b) {
					return a.current_fitness < b.current_fitness;
				}
			);

			return result->current;
		}
};
