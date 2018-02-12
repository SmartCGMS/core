/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <map>
#include <ctime>
#include <chrono>

#include "solution.h"
#include "fitness.h"

#undef max
#undef min

#include <tbb/parallel_for.h>

#include "../../../common/DebugHelper.h"


#include "NullMethod.h"
#include "LocalSearch.h"


namespace metade {

	enum class NStrategy : size_t { desCurrentToPBest = 0, desCurrentToUmPBest, desBest2Bin, desUmBest1, desCurrentToRand1, count };

	const std::map<NStrategy, const char*> strategy_name = {	{ NStrategy::desCurrentToPBest,		"CurToPBest" },
														{ NStrategy::desCurrentToUmPBest,	"CurToUmPBest" },
														{ NStrategy::desBest2Bin,			"Best2Bin" },
														{ NStrategy::desUmBest1,			"UmBest1" },
														{ NStrategy::desCurrentToRand1,		"CurToRand1" } };



	const static double Strategy_min = 0.0;
	const static double Strategy_range = 4.0;

	template <typename TSolution>
	struct TMetaDE_Candidate_Solution {
		NStrategy strategy;
		double CR, F;
		TSolution current, next;
		double current_fitness, next_fitness;
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


template <typename TSolution, typename TFitness, typename TLocalSolver, size_t mGeneration_Count = 10'000, size_t mPopulation_Size = 100>
class CMetaDE {
protected:
	//const size_t mPopulation_Size = 100;
	//const size_t mGeneration_Count = 10000;
	const size_t mPBest_Count = 5;
	const double mCR_min = 0.0;
	const double mCR_range = 1.0;
	const double mF_min = 0.0;
	const double mF_range = 2.0;
	const size_t mStrategy_min = 0;
	const size_t mStrategy_range = 4;	
protected:
	const bool mCollect_Statistics = true;
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
	 const double secs_per_iter = secs / static_cast<double>(mGeneration_Count);

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
	const TSolution mLower_Bound;
	const TSolution mUpper_Bound;
	std::vector<metade::TMetaDE_Candidate_Solution<TSolution>> mPopulation;
	std::vector<size_t> mPopulation_Best;	//indexes into mPopulation sorted by mPopulation's member's current fitness
											//we do not sort mPopulation due to performance costs and not to loose population diversity
	void Generate_Meta_Params(metade::TMetaDE_Candidate_Solution<TSolution> &solution) { 
		solution.CR = mCR_min + mCR_range*mUniform_Distribution(mRandom_Generator);
		solution.F = mF_min + mF_range*mUniform_Distribution(mRandom_Generator);
		
		const auto old_strategy = solution.strategy;
		decltype(solution.strategy) new_strategy;
		do {
			new_strategy = static_cast<metade::NStrategy>(static_cast<size_t>(metade::Strategy_min + round(metade::Strategy_range*mUniform_Distribution(mRandom_Generator))));
			new_strategy = mUniform_Distribution(mRandom_Generator) > 0.5 ? metade::NStrategy::desCurrentToUmPBest : metade::NStrategy::desCurrentToRand1;
		} while (old_strategy == new_strategy);

		solution.strategy = new_strategy;
		
		//solution.strategy = mUniform_Distribution(mRandom_Generator) > 0.5 ? metade::NStrategy::desCurrentToUmPBest : metade::NStrategy::desCurrentToRand1;
		//solution.strategy = metade::NStrategy::desCurrentToRand1;
	}
protected:
	//not used in a thread-safe way but does not seem to be a problem so far
	std::random_device mRandom_Device;
	std::mt19937 mRandom_Generator{ mRandom_Device() };
	std::uniform_real_distribution<double> mUniform_Distribution{ 0.0, 1.0 };
protected:
	TFitness &mFitness;
	glucose::SMetric &mMetric;
public:
	CMetaDE(const std::vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :
		mLower_Bound(lower_bound), mUpper_Bound(upper_bound), mFitness(fitness), mMetric(metric), mPopulation(mPopulation_Size), mPopulation_Best(mPopulation_Size) {


		//1. create the initial population
		const size_t initialized_count = std::min(mPopulation_Size / 2, initial_solutions.size());
		//a) by storing suggested params
		for (size_t i = 0; i < initialized_count; i++) {
			mPopulation[i].current = mUpper_Bound.min(mLower_Bound.max(initial_solutions[i]));//also ensure the bounds
		}

		//b) by complementing it with randomly generated numbers
		const auto bounds_range = mUpper_Bound - mLower_Bound;
		for (size_t i = initialized_count; i < mPopulation_Size; i++) {
			TSolution tmp;

			for (auto j = 0; j < bounds_range.cols(); j++)
				tmp[j] = mUniform_Distribution(mRandom_Generator);

			mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
		}

		//c) and shuffle
		std::shuffle(mPopulation.begin(), mPopulation.end(), mRandom_Generator);

		//2. set cr and f parameters, and calculate the metrics
		glucose::SMetric metric_calculator = mMetric.Clone();
		for (auto &solution : mPopulation) {
			//Do not do this parallel as the metric_calculator would have to be created many times
			Generate_Meta_Params(solution);
			solution.current_fitness = mFitness.Calculate_Fitness(solution.current, metric_calculator);
		}

		//3. finally, create and fill mpopulation indexes
		std::iota(mPopulation_Best.begin(), mPopulation_Best.end(), 0);
	}

	TSolution Solve(volatile glucose::TSolver_Progress &progress)	{

		if (mCollect_Statistics) {
			Take_Statistics_Snapshot();
			mSolve_Start_Time = std::chrono::high_resolution_clock::now();
		}

		progress.current_progress = 0;
		progress.max_progress = mGeneration_Count;

		while ((progress.current_progress++ < mGeneration_Count) && (progress.cancelled == 0)) {
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
			tbb::parallel_for(tbb::blocked_range<size_t>(size_t(0), mPopulation_Size), [=, &progress](const tbb::blocked_range<size_t> &r) {

				glucose::SMetric metric_calculator = mMetric.Clone();
				//for (size_t iter = 0; iter<mPopulation_Size; iter++) {

				const size_t rend = r.end();
				for (size_t iter = r.begin(); iter != rend; iter++) {

					auto generate_random_index = [&]()->size_t {
						const double fl_population_high = static_cast<double>(mPopulation_Size - 1);
						return static_cast<size_t>(round(mUniform_Distribution(mRandom_Generator)*fl_population_high));
					};

					auto random_difference_vector = [&]()->TSolution {
						const size_t idx1 = generate_random_index();
						const size_t idx2 = generate_random_index();
						return mPopulation[idx1].current - mPopulation[idx2].current;
					};

					auto &candidate_solution = mPopulation[iter];

					switch (candidate_solution.strategy) {
					case metade::NStrategy::desCurrentToPBest:
					{
						const size_t p_index = static_cast<size_t>(round(mUniform_Distribution(mRandom_Generator)*static_cast<double>(mPBest_Count - 1)));
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*(mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
							candidate_solution.F*random_difference_vector();
					}
					break;

					case metade::NStrategy::desCurrentToUmPBest:
					{
						const size_t p_index = static_cast<size_t>(round(mUniform_Distribution(mRandom_Generator)*static_cast<double>(mPBest_Count - 1)));
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*(mPopulation[mPopulation_Best[p_index]].current - candidate_solution.current) +
							mUniform_Distribution(mRandom_Generator)*random_difference_vector();
					}
					break;

					case metade::NStrategy::desBest2Bin:
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*random_difference_vector() +
							candidate_solution.F*random_difference_vector();
						break;

					case metade::NStrategy::desUmBest1:
						candidate_solution.next = candidate_solution.current +
							mUniform_Distribution(mRandom_Generator)*random_difference_vector() +
							candidate_solution.F*random_difference_vector();
						break;

					case metade::NStrategy::desCurrentToRand1:
						candidate_solution.next = candidate_solution.current +
							candidate_solution.F*(mPopulation[mPopulation_Best[0]].current - candidate_solution.current) +
							mUniform_Distribution(mRandom_Generator)*random_difference_vector();
						break;
					}

					//ensure the bounds				
					candidate_solution.next = mUpper_Bound.min(mLower_Bound.max(candidate_solution.next));


					//crossbreed
					{
						const size_t element_count = candidate_solution.next.cols();
						for (auto element_iter = 0; element_iter < element_count; element_iter++) {
							if (mUniform_Distribution(mRandom_Generator) > candidate_solution.CR)
								candidate_solution.next[element_iter] = candidate_solution.current[element_iter];
						}

						//and, we alway have to keep at least one original/current element
						const size_t element_to_replace = static_cast<size_t>(round(mUniform_Distribution(mRandom_Generator)*static_cast<double>(element_count - 1)));
						candidate_solution.next[element_to_replace] = candidate_solution.current[element_to_replace];
					}

/*
					//apply additional local solver
					const std::vector<TSolution> local_solution_candidate = { candidate_solution.next };
					TLocalSolver local_solver { local_solution_candidate, mLower_Bound, mUpper_Bound, mFitness, mMetric_Factory };
					TSolverProgress tmp_progress;
					candidate_solution.next = local_solver.Solve(tmp_progress);
*/
					//and evaluate
					candidate_solution.next_fitness = mFitness.Calculate_Fitness(candidate_solution.next, metric_calculator);
				}
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
		const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const metade::TMetaDE_Candidate_Solution<TSolution> &a, const metade::TMetaDE_Candidate_Solution<TSolution> &b) {return a.current_fitness < b.current_fitness; });
		return result->current;
	}
};