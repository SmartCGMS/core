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


namespace metade {
	
	enum class NStrategy : size_t { desCurrentToPBest = 0, desCurrentToUmPBest, desBest2Bin, desUmBest1, desCurrentToRand1, desTournament, count };

	const std::map<NStrategy, const char*, std::less<NStrategy>> strategy_name = {
														{ NStrategy::desCurrentToPBest,		"CurToPBest" },
														{ NStrategy::desCurrentToUmPBest,	"CurToUmPBest" },
														{ NStrategy::desBest2Bin,			"Best2Bin" },
														{ NStrategy::desUmBest1,			"UmBest1" },
														{ NStrategy::desCurrentToRand1,		"CurToRand1" },
														{ NStrategy::desTournament,			"Tournament" } };



	//const static double Strategy_min = 0.0;
	//const static double Strategy_range = static_cast<double>(static_cast<size_t>(NStrategy::count)-1);

	template <typename TUsed_Solution>
	struct TMetaDE_Candidate_Solution {
		size_t population_index = 0;	//only because of the tournament and =0 to keep static analyzer happy
		NStrategy strategy;
		double CR = 0.5, F = 1.0;
		TUsed_Solution current, next;
		double current_fitness = std::numeric_limits<double>::max();
		double next_fitness = std::numeric_limits<double>::max();
	};

	struct TMetaDE_Stats {
		double best_fitness;
		double strategy_fitness[static_cast<size_t>(NStrategy::count)];
		size_t strategy_counter[static_cast<size_t>(NStrategy::count)];
	};

	template <typename T>
	T& increment(T& value)
	{
		static_assert(std::is_integral<std::underlying_type_t<T>>::value, "Can't increment value");
		((std::underlying_type_t<T>&)value)++;
		return value;
	}
}


template <typename TUsed_Solution, typename TRandom_Device = std::random_device>//CHalton_Device>
class CMetaDE {
protected:
	static constexpr size_t mPBest_Count = 5;
	const double mCR_min = 0.0;
	const double mCR_range = 1.0;
	const double mF_min = 0.0;
	const double mF_range = 2.0;
protected:
	const bool mCollect_Statistics = false;
	const bool mPrint_Brief_Statistics = true;
	std::chrono::high_resolution_clock::time_point mSolve_Start_Time, mSolve_Stop_Time;

	std::vector<metade::TMetaDE_Stats> mStatistics;

	void Take_Statistics_Snapshot(void) {
		//just takes a current snapshot of the population
		metade::TMetaDE_Stats snapshot;
		for (size_t j = 0; j < static_cast<size_t>(metade::NStrategy::count); j++) {
			snapshot.strategy_counter[j] = 0;
			snapshot.strategy_fitness[j] = std::numeric_limits<double>::max();
		}
		snapshot.best_fitness = std::numeric_limits<double>::max();

		for (const auto &individual : mPopulation) {
			snapshot.strategy_counter[static_cast<size_t>(individual.strategy)]++;
			snapshot.strategy_fitness[static_cast<size_t>(individual.strategy)] = std::min(snapshot.strategy_fitness[static_cast<size_t>(individual.strategy)], individual.current_fitness);
			snapshot.best_fitness = std::min(snapshot.best_fitness, individual.current_fitness);
		}

		mStatistics.push_back(snapshot);
	}

	void Print_Statistics() {
		if (mPrint_Brief_Statistics) Print_Statistics_Brief();
		else Print_Statistics_Full();  
	}

	void Print_Statistics_Brief() {
		//find the least fitness value we ever observed
		const auto best_fitness_iter = std::min_element(mStatistics.begin(), mStatistics.end(), [](const auto &left, const auto &right){ return left.best_fitness < right.best_fitness; });
		const auto best_fitness_dist = std::distance(mStatistics.begin(), best_fitness_iter);

		std::chrono::duration<double, std::milli> secs_duration = mSolve_Stop_Time - mSolve_Start_Time;
		const double secs = secs_duration.count()*0.001;
		const double secs_per_iter = secs / static_cast<double>(mSetup.max_generations);

		dprintf((char*)"%g; %d; %f; %f\n", best_fitness_iter->best_fitness, best_fitness_dist, secs, secs_per_iter);
	}

	void Print_Statistics_Full() {
		dprintf("\n\nStatistics begin\n\n");

		dprintf("Iteration; best");
		for (auto i = static_cast<metade::NStrategy>(0); i < metade::NStrategy::count; metade::increment(i))
			dprintf((char*)"; %s_fit", (char*) metade::strategy_name.find(i)->second);


		for (auto i = static_cast<metade::NStrategy>(0); i < metade::NStrategy::count; metade::increment(i))
			dprintf((char*)"; %s_cnt", (char*) metade::strategy_name.find(i)->second);


		for (size_t i = 0; i < mStatistics.size(); i++) {
			dprintf((char*)"\n%d; %g", i, mStatistics[i].best_fitness);

			for (size_t j = 0; j < static_cast<size_t>(metade::NStrategy::count); j++)
				dprintf((char*)"; %g", mStatistics[i].strategy_fitness[j]);

			for (size_t j = 0; j < static_cast<size_t>(metade::NStrategy::count); j++)
				dprintf((char*)"; %d", mStatistics[i].strategy_counter[j]);

		}
    
		dprintf("\n\nBest fit; iter; secs; secs/iter\n");
		Print_Statistics_Brief();

		dprintf("\n\nStatistics end\n\n");
	}
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
	TAligned_Solution_Vector<metade::TMetaDE_Candidate_Solution<TUsed_Solution>> mPopulation;
	std::vector<size_t> mPopulation_Best;	//indexes into mPopulation sorted by mPopulation's member's current fitness
											//we do not sort mPopulation due to performance costs and not to loose population diversity
	void Generate_Meta_Params(metade::TMetaDE_Candidate_Solution<TUsed_Solution> &solution) { 
		solution.CR = mCR_min + mCR_range*mUniform_Distribution_dbl(mRandom_Generator);
		solution.F = mF_min + mF_range*mUniform_Distribution_dbl(mRandom_Generator);

		const auto old_strategy = solution.strategy;
		decltype(solution.strategy) new_strategy;
		do {
			new_strategy = static_cast<metade::NStrategy>(mUniform_Distribution_Strategy(mRandom_Generator));							
			//new_strategy = mUniform_Distribution(mRandom_Generator) > 0.5 ? metade::NStrategy::desCurrentToUmPBest : metade::NStrategy::desCurrentToRand1;
		} while (old_strategy == new_strategy);

		solution.strategy = new_strategy;
	}
protected:
	//not used in a thread-safe way but does not seem to be a problem so far
	//std::random_device mRandom_Device;
	//std::mt19937 mRandom_Generator{ mRandom_Device() };
	inline static TRandom_Device mRandom_Generator;
	inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };
	inline static thread_local std::uniform_int_distribution<size_t> mUniform_Distribution_PBest{ 0, mPBest_Count-1 };
	inline static thread_local std::uniform_int_distribution<size_t> mUniform_Distribution_Strategy{ 0, static_cast<size_t>(metade::NStrategy::count)-1 };
protected:
    solver::TSolver_Setup mSetup;
public:
	CMetaDE(const solver::TSolver_Setup &setup) : 
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
			TUsed_Solution tmp;

			// this helps when we use generic solution vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
			// optimized away in compile time
			tmp.resize(Eigen::NoChange, mSetup.problem_size);

			for (size_t j = 0; j < mSetup.problem_size; j++)
				tmp[j] = mUniform_Distribution_dbl(mRandom_Generator);

			mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
		}

		//c) and shuffle
		std::shuffle(mPopulation.begin(), mPopulation.end(), mRandom_Generator);

		//2. set cr and f parameters, and calculate the metrics		
		for (size_t i = 0; i < mPopulation.size(); i++) {
			auto &solution = mPopulation[i];
			Generate_Meta_Params(solution);
			solution.population_index = i;
			solution.current_fitness = mSetup.objective(mSetup.data, solution.current.data());
			
		}

		//3. finally, create and fill mpopulation indexes
		std::iota(mPopulation_Best.begin(), mPopulation_Best.end(), 0);
	}


	TUsed_Solution Solve(solver::TSolver_Progress &progress) {

		if (mCollect_Statistics) {
			Take_Statistics_Snapshot();
			mSolve_Start_Time = std::chrono::high_resolution_clock::now();
		}

		progress.current_progress = 0;
		progress.max_progress = mSetup.max_generations;
	
		const size_t solution_size = mPopulation[0].current.cols();
		std::uniform_int_distribution<size_t> mUniform_Distribution_Solution{ 0, solution_size - 1 };	//this distribution must be local as solution can be dynamic
		std::uniform_int_distribution<size_t> mUniform_Distribution_Population{ 0, mPopulation.size() - 1 };

		while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == FALSE)) {
			//1. determine the best p-count parameters, without actually re-ordering the population
			//we want to avoid of getting all params close together and likely loosing the population diversity
			std::partial_sort(mPopulation_Best.begin(), mPopulation_Best.begin() + mPBest_Count, mPopulation_Best.end(),
				[&](const size_t &a, const size_t &b) {return mPopulation[a].current_fitness < mPopulation[b].current_fitness; });

			//update the progress
			progress.best_metric = mPopulation[mPopulation_Best[0]].current_fitness;

			//2. Calculate the next vectors and their fitness 
			//In this step, current is read-only and next is write-only => no locking is needed
			//as each next will be written just once.
			//We assume that parallelization cost will get amortized			

			std::for_each(std::execution::par_unseq, mPopulation.begin(), mPopulation.end(), [=, &mUniform_Distribution_Solution, &mUniform_Distribution_Population](auto &candidate_solution) {

			
				auto random_difference_vector = [&]()->TUsed_Solution {
					const size_t idx1 = mUniform_Distribution_Population(mRandom_Generator);
					const size_t idx2 = mUniform_Distribution_Population(mRandom_Generator);
					return mPopulation[idx1].current - mPopulation[idx2].current;
				};

				//auto &candidate_solution = mPopulation[iter];

				switch (candidate_solution.strategy) {
					case metade::NStrategy::desCurrentToPBest:
					{
						const size_t p_index = mUniform_Distribution_PBest(mRandom_Generator);
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*(mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
							candidate_solution.F*random_difference_vector();
					}
					break;

					case metade::NStrategy::desCurrentToUmPBest:
					{
						const size_t p_index = mUniform_Distribution_PBest(mRandom_Generator);
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*(mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
							mUniform_Distribution_dbl(mRandom_Generator)*random_difference_vector();
					}
					break;

					case metade::NStrategy::desBest2Bin:
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*random_difference_vector() +
							candidate_solution.F*random_difference_vector();
						break;

					case metade::NStrategy::desUmBest1:
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*(mPopulation[mPopulation_Best[0]].current - candidate_solution.current) +
							mUniform_Distribution_dbl(mRandom_Generator)*random_difference_vector();
						break;

					case metade::NStrategy::desCurrentToRand1:
						candidate_solution.next = candidate_solution.current +
							mUniform_Distribution_dbl(mRandom_Generator)*random_difference_vector() +
							candidate_solution.F*random_difference_vector();
						break;

					case metade::NStrategy::desTournament: {
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
						}
						break;

					default:
						break;
				}

				//ensure the bounds
				candidate_solution.next = mUpper_Bound.min(mLower_Bound.max(candidate_solution.next));


				//crossbreed
				{						
					for (size_t element_iter = 0; element_iter < solution_size; element_iter++) {
						if (mUniform_Distribution_dbl(mRandom_Generator) > candidate_solution.CR)
							candidate_solution.next[element_iter] = candidate_solution.current[element_iter];
					}

					//and, we alway have to keep at least one original/current element
					const size_t element_to_replace = mUniform_Distribution_Solution(mRandom_Generator);
					candidate_solution.next[element_to_replace] = candidate_solution.current[element_to_replace];
				}

				//and evaluate					
				candidate_solution.next_fitness = mSetup.objective(mSetup.data, candidate_solution.next.data());						
				
			});


			//3. Let us preserve the better vectors - too fast to amortize parallelization => serial code
			for (auto &solution : mPopulation) {
				//used strategy produced a better offspring => leave meta params as they are
				if (solution.current_fitness > solution.next_fitness) {
					solution.current = solution.next;
					solution.current_fitness = solution.next_fitness;
				}
				else {
					//the offspring is worse than its parents => modify parents' DE parameters
					Generate_Meta_Params(solution);
				}
			}

			if (mCollect_Statistics) Take_Statistics_Snapshot();
		} //while in the progress

		if (mCollect_Statistics) {
			mSolve_Stop_Time = std::chrono::high_resolution_clock::now();
			Print_Statistics();
		}

		//find the best result and return it
		const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const metade::TMetaDE_Candidate_Solution<TUsed_Solution> &a, const metade::TMetaDE_Candidate_Solution<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
		return result->current;		
	}
};