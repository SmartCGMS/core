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

#undef max
#undef min

#include "solution.h"

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/Eigen_Buffer.h"
#include "../../../common/utils/DebugHelper.h"


namespace metade {
	
	//Mutation strategy
	enum class NStrategy : size_t { desPolynomial = 0, desSBX_Children, desCurrentToPBest, desCurrentToUmPBest, desBest2Bin, desUmBest1, desCurrentToRand1, desTournament, desSBX_Alike_Random, desSBX_Alike_PBest, count };

	const std::map<NStrategy, const char*, std::less<NStrategy>> strategy_name = {
														{ NStrategy::desCurrentToPBest,		"CurToPBest" },
														{ NStrategy::desCurrentToUmPBest,	"CurToUmPBest" },
														{ NStrategy::desBest2Bin,			"Best2Bin" },
														{ NStrategy::desUmBest1,			"UmBest1" },
														{ NStrategy::desCurrentToRand1,		"CurToRand1" },
														{ NStrategy::desTournament,			"Tournament" },
														{ NStrategy::desSBX_Alike_Random,	"SBXalikeRand" },
														{ NStrategy::desSBX_Alike_PBest,	"SBXalikePBest" },
														{ NStrategy::desPolynomial,			"Polynomial"},
														{ NStrategy::desSBX_Children,		"SBX_Single"},
}; 



	//const static double Strategy_min = 0.0;
	//const static double Strategy_range = static_cast<double>(static_cast<size_t>(NStrategy::count)-1);

	template <typename TUsed_Solution>
	struct TMetaDE_Candidate_Solution {
		TUsed_Solution current;		//declared as first to aid the alignement

		size_t population_index = 0;	//only because of the tournament and =0 to keep static analyzer happy
		NStrategy strategy = NStrategy::desCurrentToPBest;
		double CR = 0.5, F = 1.0;
		size_t strategy_TTL = 5;
		
		//TMapped_Solution<TUsed_Solution> next;
		
		NFitness_Strategy fitness_strategy = NFitness_Strategy::Master;

		solver::TFitness current_fitness{ solver::Nan_Fitness };
		solver::TFitness *next_fitness = nullptr;
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
			snapshot.strategy_fitness[static_cast<size_t>(individual.strategy)] = std::min(snapshot.strategy_fitness[static_cast<size_t>(individual.strategy)], individual.current_fitness[0]);
			snapshot.best_fitness = std::min(snapshot.best_fitness, individual.current_fitness[0]);
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
	using TCandidate_Solution = metade::TMetaDE_Candidate_Solution<TUsed_Solution>;
	std::vector<TCandidate_Solution, AlignmentAllocator<TCandidate_Solution>> mPopulation;
	std::vector<size_t> mPopulation_Best;	//indexes into mPopulation sorted by mPopulation's member's current fitness
											//we do not sort mPopulation due to performance costs and not to loose population diversity

	const size_t Max_Strategy_TTL = 10;

	template <typename T>
	T Generate_New_Strategy(const T old_strategy, std::uniform_int_distribution<size_t> &distribution) {
		T new_strategy;

		do {
			new_strategy = static_cast<T>(distribution(mRandom_Generator));			
		} while (old_strategy == new_strategy);

		return new_strategy;
	}

	void Generate_Meta_Params(TCandidate_Solution &solution) {
		solution.CR = mCR_min + mCR_range*mUniform_Distribution_dbl(mRandom_Generator);
		solution.F = mF_min + mF_range*mUniform_Distribution_dbl(mRandom_Generator);

		if (solution.strategy_TTL > 0) {
			solution.strategy_TTL--;		//CR and FR paremeters may have been wrong only, do not change strategy so soon
		}
		else {
			solution.strategy = Generate_New_Strategy<metade::NStrategy>(solution.strategy, mUniform_Distribution_Strategy);
			solution.fitness_strategy = Generate_New_Strategy<NFitness_Strategy>(solution.fitness_strategy, mUniform_Distribution_Fitness_Strategy);
			solution.strategy_TTL = Max_Strategy_TTL / 2;
		}
	}
protected:
	std::vector<double, AlignmentAllocator<double>> mNext_Solutions, mNext_Fitnesses;
		//these are the containers, which allow bulk objective calls - Eigen::Map does not work due to missing construct_at

	void Store_Next_Solution(const size_t index, TUsed_Solution& dst) const {
		const auto begin = Next_Solution(index);
		std::copy(begin, begin + mSetup.problem_size, dst.data());
	}


	double* Next_Solution(const size_t index) const {
		return const_cast<double*>(mNext_Solutions.data() + index * mSetup.problem_size);
	}

protected:
	//not used in a thread-safe way but does not seem to be a problem so far
	//std::random_device mRandom_Device;
	//std::mt19937 mRandom_Generator{ mRandom_Device() };
	inline static TRandom_Device mRandom_Generator;
	inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };
	inline static thread_local std::uniform_int_distribution<size_t> mUniform_Distribution_PBest{ 0, mPBest_Count-1 };
	inline static thread_local std::uniform_int_distribution<size_t> mUniform_Distribution_Strategy{ 0, static_cast<size_t>(metade::NStrategy::count)-1 };
	inline static thread_local std::uniform_int_distribution<size_t> mUniform_Distribution_Fitness_Strategy{ 0, static_cast<size_t>(NFitness_Strategy::count) - 1 };
protected:
	//http://www.iitk.ac.in/kangal/codes/nsga2/nsga2-gnuplot-v1.1.6.tar.gz
	// http://www.slideshare.net/paskorn/simulated-binary-crossover-presentation
	TUsed_Solution Polynomial_Mutation(TCandidate_Solution& solution) const {

		TUsed_Solution result = solution.current;
		result.eval();

		for (size_t i = 0; i < mSetup.problem_size; i++) {			
			const double bounds_diff = mSetup.upper_bound[i] - mSetup.lower_bound[i];
			if (bounds_diff > 0.0) {
				

				if (mUniform_Distribution_dbl(mRandom_Generator) < solution.CR) {
					double y = solution.current[i];

					const double delta1 = (y - lb) / bounds_diff;
					const double delta2 = (ub - y) / bounds_diff;

					const double eta = 20.0;
					const double mut_pow = 1.0 / (eta + 1.0);

					double deltaq = 0.0;
					const double rnd = mUniform_Distribution_dbl(mRandom_Generator);
					if (rnd <= 0.5) {
						const double xy = 1.0 - delta1;
						const double val = 2.0 * rnd + (1.0 - 2.0 * rnd) * (pow(xy, (eta + 1.0)));
						deltaq = pow(val, mut_pow) - 1.0;
					}
					else {
						const double xy = 1.0 - delta2;
						const double val = 2.0 * (1.0 - rnd) + 2.0 * (rnd - 0.5) * (pow(xy, (eta + 1.0)));
						deltaq = 1.0 - (pow(val, mut_pow));
					}

					y = y + deltaq * bounds_diff;
					//y = std::min(ub, std::max(lb, y)); done later in a common fashion

					result[i] = y;
				}
			}
		}


		return result;
	}

	double get_SBX_betaq(double rand, double alpha, double eta) const{
		return rand <= (1.0 / alpha) 
			? std::pow((rand * alpha), (1.0 / (eta + 1.0)))
			: std::pow((1.0 / (2.0 - rand * alpha)), (1.0 / (eta + 1.0)));
	}


	TUsed_Solution SBX_Children(const TCandidate_Solution &parent1, const TCandidate_Solution &parent2) const {		
		const double eta = 30.0;// NSGA-III (t-EC 2014) setting

		const bool left_child = ((0.5*(parent1.F + parent2.F)- mF_min) < 0.5 * mF_range);	//unlike original SBX, we can produce just one children, so we need to choose which one
		TUsed_Solution result = left_child ? parent1.current : parent2.current;

		if (parent1.current[i] != parent2.current[i]) {
			for (size_t i = 0; i < mSetup.problem_size; i++) {
				if (mUniform_Distribution_dbl(mRandom_Generator) > 0.5) continue; // these two variables are not crossovered



				const double y1 = std::min(parent1.current[i], parent2.current[i]);
				const double y2 = std::max(parent1.current[i], parent2.current[i]);

				if (y2 > y1) {

					const double lb = mSetup.lower_bound[i];
					const double ub = mSetup.upper_bound[i];

					const double rand = mUniform_Distribution_dbl(mRandom_Generator);


					bool effective_left_child = mUniform_Distribution_dbl(mRandom_Generator) < 0.5 ? !left_child : left_child; //swap if random			

					if (effective_left_child) {
						const double beta = 1.0 + (2.0 * (y1 - lb) / (y2 - y1));
						const double alpha = 2.0 - std::pow(beta, -(eta + 1.0));
						const double betaq = get_SBX_betaq(rand, alpha, eta);

						result[i] = 0.5 * ((y1 + y2) - betaq * (y2 - y1));
					}
					else {
						const double beta = 1.0 + (2.0 * (ub - y2) / (y2 - y1));
						const double alpha = 2.0 - std::pow(beta, -(eta + 1.0));
						const double betaq = get_SBX_betaq(rand, alpha, eta);

						result[i] = 0.5 * ((y1 + y2) + betaq * (y2 - y1));
					}
				}

			}
		}

		return result;
	}

protected:
    solver::TSolver_Setup mSetup;
public:
	CMetaDE(const solver::TSolver_Setup &setup) : 
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
		mSetup(solver::Check_Default_Parameters(setup, 100'000, 100)) {

		mPopulation.resize(std::max(mSetup.population_size, mPBest_Count));
		mPopulation_Best.resize(mPopulation.size());
		const size_t initialized_count = std::min(mPopulation.size() / 2, mSetup.hint_count);

		mNext_Solutions.resize(mSetup.problem_size * mSetup.population_size);
		mNext_Fitnesses.resize(solver::Maximum_Objectives_Count * mSetup.population_size);
		std::fill(mNext_Fitnesses.begin(), mNext_Fitnesses.end(), std::numeric_limits<double>::quiet_NaN());


		//1. create the initial population		
		std::vector<TUsed_Solution> trimmed_hints;
		std::vector<size_t> hint_indexes(mSetup.hint_count);
		std::vector<solver::TFitness> hint_fitness(mSetup.hint_count, solver::Nan_Fitness);
		std::vector<bool> hint_validity(mSetup.hint_count);

		//a) find the best hints
		// trim the parameters to the bounds
		std::iota(std::begin(hint_indexes), std::end(hint_indexes), 0);
		for (size_t i = 0; i < setup.hint_count; i++) {
			trimmed_hints.push_back(mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size))));//ensure the bounds
		}

		//b check their fitness in parallel 		
		std::for_each(std::execution::par_unseq, hint_indexes.begin(), hint_indexes.end(), [this, &trimmed_hints, &hint_fitness, &hint_validity](auto& hint_idx) {
			hint_validity[hint_idx] = mSetup.objective(mSetup.data, 1, trimmed_hints[hint_idx].data(), hint_fitness[hint_idx].data()) == TRUE;
		});

		if (initialized_count < mSetup.hint_count) { 
			//and sort the select up to the initialized_count best of them - if actually needed
			std::partial_sort(hint_indexes.begin(), hint_indexes.begin() + initialized_count, hint_indexes.end(),
				[&](const size_t& a, const size_t& b) {
					if (!hint_validity[a]) return false;
					if (!hint_validity[b]) return true;

					return Compare_Solutions(hint_fitness[a], hint_fitness[b], mSetup.objectives_count, NFitness_Strategy::Master);
							//true/false domination see the sorting in the main cycle
				});
		}
		

		//1. create the initial population		
		
				
		//b) by storing suggested params
		size_t effectively_initialized_count = 0;
		for (size_t i = 0; i < initialized_count; i++) {
			if (hint_validity[hint_indexes[i]]) {
				effectively_initialized_count++;
				mPopulation[i].current = trimmed_hints[hint_indexes[i]];
				mPopulation[i].current_fitness = hint_fitness[hint_indexes[i]];
			}
			else
				break;
		}

		//c) by complementing it with randomly generated numbers
		const auto bounds_range = mUpper_Bound - mLower_Bound;
		for (size_t i = effectively_initialized_count; i < mPopulation.size(); i++) {
			TUsed_Solution tmp;

			// this helps when we use generic dst vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
			// optimized away in compile time
			tmp.resize(Eigen::NoChange, mSetup.problem_size);

			for (size_t j = 0; j < mSetup.problem_size; j++)
				tmp[j] = mUniform_Distribution_dbl(mRandom_Generator);

			mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
		}

		//compute the fitness in parallel
		std::for_each(std::execution::par_unseq, mPopulation.begin() + effectively_initialized_count, mPopulation.end(), [this](auto& candidate_solution) {
			if (mSetup.objective(mSetup.data, 1, candidate_solution.current.data(), candidate_solution.current_fitness.data()) != TRUE) {
				for (auto& elem : candidate_solution.current_fitness)
					elem = std::numeric_limits<double>::quiet_NaN();
			  }
			});


		//d) and shuffle
		std::shuffle(mPopulation.begin(), mPopulation.end(), mRandom_Generator);

		//2. set cr and f parameters, and reset the unused part of the metrics
		//also, setup the next metric and next pointers
		for (size_t i = 0; i < mPopulation.size(); i++) {
			auto &solution = mPopulation[i];
			Generate_Meta_Params(solution);
			solution.population_index = i;

			//solution.current_fitness = solver::Nan_Fitness;	//next fitness is dereferenced only				
			solution.next_fitness = reinterpret_cast<solver::TFitness*>(&mNext_Fitnesses[i * solver::Maximum_Objectives_Count]);
			*solution.next_fitness = solution.current_fitness;
			//dst.next = std::move(Map_Double_To_Eigen<TUsed_Solution>(&mNext_Solutions[i * mSetup.problem_size], mSetup.problem_size));
		}

		

		//3. finally, create and fill mpopulation indexes
		std::iota(mPopulation_Best.begin(), mPopulation_Best.end(), 0);
	}


	TUsed_Solution Solve(solver::TSolver_Progress &progress) {

		if (mCollect_Statistics) {
			Take_Statistics_Snapshot();
			mSolve_Start_Time = std::chrono::high_resolution_clock::now();
		}

		progress = solver::Null_Solver_Progress;
		progress.current_progress = 0;
		progress.max_progress = mSetup.max_generations;
	
		const size_t solution_size = mPopulation[0].current.cols();
		std::uniform_int_distribution<size_t> mUniform_Distribution_Solution{ 0, solution_size - 1 };	//this distribution must be local as dst can be dynamic
		std::uniform_int_distribution<size_t> mUniform_Distribution_Population{ 0, mPopulation.size() - 1 };

		const size_t partial_sort_size = std::max(mPBest_Count, mPopulation.size() / 2);

		while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == FALSE)) {
			//1. determine the best p-count parameters, without actually re-ordering the population
			//we want to avoid of getting all params close together and likely loosing the population diversity
			//std::partial_sort(mPopulation_Best.begin(), mPopulation_Best.begin() + mPBest_Count, mPopulation_Best.end(),
			
			std::partial_sort(mPopulation_Best.begin(), mPopulation_Best.begin() + partial_sort_size, mPopulation_Best.end(),
				[&](const size_t &a, const size_t &b) {
			
				//comparison to NaN would yield false, if std::numeric_limits::is_iec559
				//otherwise, it is implementation specific
				return Compare_Solutions(mPopulation[a].current_fitness, mPopulation[b].current_fitness, mSetup.objectives_count, NFitness_Strategy::Master);
					//do not require strict domination as the [0] has to be the best one, so we need to choose from the non-dominating set
			});			


			//update the progress
			progress.best_metric = mPopulation[mPopulation_Best[0]].current_fitness;

			//2. Calculate the next vectors and their fitness 
			//In this step, current is read-only and next is write-only => no locking is needed
			//as each next will be written just once.
			//We assume that parallelization cost will get amortized			

			//std::for_each(std::execution::unseq, mPopulation.begin(), mPopulation.end(), [=, &mUniform_Distribution_Solution, &mUniform_Distribution_Population](auto &candidate_solution) {

			TUsed_Solution intermediate;		//this one would need to be inside the cycle, if the cycle would be parallelized again
			intermediate.resize(Eigen::NoChange, mSetup.problem_size);
			for  (auto& candidate_solution : mPopulation) { //std for each seems to make a bug, at least with VS2019
			
				//in the original version, which did not support the bulk objective call, this for-cycle used to be parallel
				//however, with bulk objective and modern processor, breeding new candidates won't amortize => just a vectorized for-cycle
				//=> no mutexes at all, only atomic ops are allowed!

				auto random_difference_vector = [&]()->TUsed_Solution {
					const size_t idx1 = mUniform_Distribution_Population(mRandom_Generator);
					const size_t idx2 = mUniform_Distribution_Population(mRandom_Generator);
					return mPopulation[idx1].current - mPopulation[idx2].current;
				};

				auto SBX_alike = [&](const bool rand)->TUsed_Solution {
					const size_t p_index = rand ?						
												mUniform_Distribution_Population(mRandom_Generator) :
												mUniform_Distribution_PBest(mRandom_Generator);
					const auto& p_elem = mPopulation[mPopulation_Best[p_index]].current;
					const double FT = mF_min + 2.0 * (candidate_solution.F - mF_min - 0.5 * mF_range);		//allow plus and minus, like with the original SBX, when creating two children
					const TUsed_Solution im = 0.5 * (candidate_solution.current + p_elem) +
						FT * (candidate_solution.current - p_elem);
						//like SBX, but we do not create two children, but just one childr is possible with the current design


					return im;

				};
							

				switch (candidate_solution.strategy) {
					case metade::NStrategy::desCurrentToPBest:
						{
							const size_t p_index = mUniform_Distribution_PBest(mRandom_Generator);
							intermediate = candidate_solution.current +
								candidate_solution.F * (mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
								candidate_solution.F * random_difference_vector();							
						}
					break;

					case metade::NStrategy::desCurrentToUmPBest:
						{
							const size_t p_index = mUniform_Distribution_PBest(mRandom_Generator);
							intermediate = candidate_solution.current +
								candidate_solution.F*(mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
								mUniform_Distribution_dbl(mRandom_Generator)*random_difference_vector();							

						}
					break;

					case metade::NStrategy::desBest2Bin: 
						{
							intermediate = candidate_solution.current +
								candidate_solution.F * random_difference_vector() +
								candidate_solution.F * random_difference_vector();														
						}
						break;

					case metade::NStrategy::desUmBest1:
						{
							intermediate = candidate_solution.current +
								candidate_solution.F*(mPopulation[mPopulation_Best[0]].current - candidate_solution.current) +
								mUniform_Distribution_dbl(mRandom_Generator)*random_difference_vector();
							
						}
						break;

					case metade::NStrategy::desCurrentToRand1:
						{
							intermediate = candidate_solution.current +
								mUniform_Distribution_dbl(mRandom_Generator)*random_difference_vector() +
								candidate_solution.F*random_difference_vector();							
						}
						break;

					case metade::NStrategy::desTournament: {
							//we choose mPBest_Count-number of random candidates for a direct crossbreeding with the current candidate dst

							size_t to_go = std::max(mPBest_Count, mPopulation.size() / 20); //5%
							size_t best_tournament_index = candidate_solution.population_index;							
							solver::TFitness best_tournament_fitness{ solver::Nan_Fitness };

							std::set<size_t> visited_tournament_indexes{ best_tournament_index };
							while (to_go-- > 0) {
								size_t random_tournament_index = mUniform_Distribution_Population(mRandom_Generator);
								if (visited_tournament_indexes.find(random_tournament_index) == visited_tournament_indexes.end()) {
									visited_tournament_indexes.insert(random_tournament_index);

									if (Compare_Solutions(mPopulation[random_tournament_index].current_fitness, best_tournament_fitness, mSetup.objectives_count, NFitness_Strategy::Master)) {
												//no need to require strict domination, as we are computing the candidate fitess now
										best_tournament_fitness = mPopulation[random_tournament_index].current_fitness;
										best_tournament_index = random_tournament_index;
									}
								}
							}
							
							intermediate = mPopulation[best_tournament_index].current;
						}
						break;

					case metade::NStrategy::desSBX_Alike_Random:
						intermediate = SBX_alike(true);
						break;
						
					case metade::NStrategy::desSBX_Alike_PBest:
						intermediate = SBX_alike(false);
						break;

					case metade::NStrategy::desPolynomial:
						intermediate = Polynomial_Mutation(candidate_solution);
						break;

					case metade::NStrategy::desSBX_Children:
						{
							const size_t p_index = mUniform_Distribution_PBest(mRandom_Generator);
							intermediate = SBX_Children(candidate_solution, mPopulation[p_index]);
						}
						break;

					default:
						break;
				}

				//ensure the bounds
				intermediate = mUpper_Bound.min(mLower_Bound.max(intermediate));
				


				//crossbreed aka recombination
				//it does not make sense for NSGA mutations, which already do it
				if ((candidate_solution.strategy != metade::NStrategy::desPolynomial) && (candidate_solution.strategy != metade::NStrategy::desSBX_Children)) {						
					for (size_t element_iter = 0; element_iter < solution_size; element_iter++) {
						if (mUniform_Distribution_dbl(mRandom_Generator) > candidate_solution.CR)
							intermediate[element_iter] = candidate_solution.current[element_iter];
					}

					//and, we alway have to keep at least one original/current element
					const size_t element_to_replace = mUniform_Distribution_Solution(mRandom_Generator);
					intermediate[element_to_replace] = candidate_solution.current[element_to_replace];
				}

				//and write				
				intermediate.eval();
				std::copy(intermediate.data(), intermediate.data() + mSetup.problem_size, Next_Solution(candidate_solution.population_index));
			
			}//);
			
			//flush all subnormals to zero
			for (auto& elem : mNext_Solutions) {
				if (std::fpclassify(elem) == FP_SUBNORMAL)
					elem = 0.0;
			}

			//and evaluate					
			if (mSetup.objective(mSetup.data, mPopulation.size(), mNext_Solutions.data(), mNext_Fitnesses.data()) != TRUE) {
				progress.cancelled = TRUE;	//error!
				break;
			}


			//3. Let us preserve the better vectors - too fast to amortize parallelization => serial code
			for (auto &solution : mPopulation) {
				//used strategy produced a better offspring => leave meta params as they are
				const bool best_individual = solution.population_index == mPopulation_Best[0];
				const auto fitness_strategy = best_individual ? NFitness_Strategy::Master : solution.fitness_strategy; //if we would allow any strategy for the best,
																													   //then, the best distance from the origin could degrade
				if (Compare_Solutions(*solution.next_fitness, solution.current_fitness, mSetup.objectives_count, fitness_strategy)) {
									//requires strict domination to push the population towards the Pareto front, but only a half of it to improve the diversity
					//dst.current = dst.next;
					Store_Next_Solution(solution.population_index, solution.current);
					solution.current_fitness = *solution.next_fitness;
					solution.strategy_TTL = std::max(Max_Strategy_TTL, solution.strategy_TTL + 1);	//increase the chances of keeping a working strategy
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
		const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const metade::TMetaDE_Candidate_Solution<TUsed_Solution> &a, const metade::TMetaDE_Candidate_Solution<TUsed_Solution> &b) {
			return Compare_Solutions(a.current_fitness, b.current_fitness, mSetup.objectives_count, NFitness_Strategy::Master);
		});

		//check for a fail
		for (size_t i = 0; i < mSetup.objectives_count; i++) {
			if (std::isnan(result->current_fitness[i])) {
				progress.cancelled = TRUE;
				break;
			}
		}

		progress.best_metric = result->current_fitness;
		return result->current;		
	}
};