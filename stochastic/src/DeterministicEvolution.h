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

//Implementation of fast, low-energy cost linear version of the Bayes optimization
//For advanced Bayes, optmization we should consider using the following toolkits (Limbo claims to be faster), which, however, both require Boost
//https://github.com/resibots/limbo
//https://github.com/rmcantin/bayesopt

#pragma once


#include "solution.h"
#include "fitness.h"
#include <random>
#include <tbb/parallel_reduce.h>


namespace Deterministic_Evolution_internal {
	template <typename TSolution>
	struct TCandidate {
		TSolution current, next;
		TSolution direction;
		double velocity;
		double current_fitness, next_fitness;
		glucose::SMetric metric_calculator;	//de-facto, we create a pool of metrics so that we don't need to clone it for each thread or implement pushing and poping to and from the pool
	};
}

#undef min

template <typename TSolution, typename TFitness, size_t mPopulation_Size = 15, size_t mGeneration_Count = 20000>
class CDeterministic_Evolution {
protected:
	TFitness &mFitness;
	glucose::SMetric &mMetric;
protected:
	const double mInitial_Velocity = 1.0 / 3.0;
	const double mVelocity_Increase = 1.05;
protected:
	TAligned_Solution_Vector<Deterministic_Evolution_internal::TCandidate<TSolution>> mPopulation;
protected:
	const TSolution mLower_Bound;
	const TSolution mUpper_Bound;
public:
	CDeterministic_Evolution(const TAligned_Solution_Vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :
		mLower_Bound(lower_bound), mUpper_Bound(upper_bound), mPopulation(mPopulation_Size), mFitness(fitness), mMetric(metric) {


		//1. create the initial population
		const size_t initialized_count = std::min(mPopulation_Size / 2, initial_solutions.size());
		//a) by storing suggested params
		for (size_t i = 0; i < initialized_count; i++) {
			mPopulation[i].current = mUpper_Bound.min(mLower_Bound.max(initial_solutions[i]));//also ensure the bounds
		}

		//b) by complementing it with randomly generated numbers
		std::mt19937 MT_sequence;	//to be completely deterministic in every run we used the constant, default seed
		const auto bounds_range = mUpper_Bound - mLower_Bound;
		for (size_t i = initialized_count; i < mPopulation_Size; i++) {
			TSolution tmp;

			// this helps when we use generic solution vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
			// optimized away in compile time
			tmp.resize(Eigen::NoChange, bounds_range.cols());

			for (auto j = 0; j < bounds_range.cols(); j++)
				tmp[j] = MT_sequence();

			mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
		}

		//2. and calculate the metrics
		for (auto &solution : mPopulation) {
			solution.metric_calculator = mMetric.Clone(); //verify that clone is thread-safe if ever reworking this to parallel (but likely not needed for a performance reasons)
			solution.current_fitness = mFitness.Calculate_Fitness(solution.current, solution.metric_calculator);
			
			//the main solving algorithm expects that next contains valid values from the last run of the generation
			solution.next = solution.current;
			solution.next_fitness = solution.current_fitness;
			
			solution.direction.setConstant(1.0);		//full speed ahead
			solution.velocity = mInitial_Velocity;
		}		
	};

	TSolution Solve(volatile glucose::TSolver_Progress &progress) {
		progress.current_progress = 0;
		progress.max_progress = mGeneration_Count;

		const size_t solution_size = mPopulation[0].next.cols();
		

		while ((progress.current_progress++ < mGeneration_Count) && (progress.cancelled == 0)) {
			
			//update the progress
			const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const Deterministic_Evolution_internal::TCandidate<TSolution> &a, const Deterministic_Evolution_internal::TCandidate<TSolution> &b) {return a.current_fitness < b.current_fitness; });
			progress.best_metric = result->current_fitness;

			//2. Calculate the next vectors and their fitness 
			//In this step, current is read-only and next is write-only => no locking is needed
			//as each next will be written just once.
			//We assume that parallelization cost will get amortized
			tbb::parallel_for(tbb::blocked_range<size_t>(size_t(0), mPopulation_Size), [=](const tbb::blocked_range<size_t> &r) {
			

				const size_t rend = r.end();
				for (size_t iter = r.begin(); iter != rend; iter++) {


					auto &candidate_solution = mPopulation[iter];

					//try to advance the current solution in the direction of the other solutions
					
					for (size_t pop_iter = 0; pop_iter < mPopulation_Size; pop_iter++) {
						TSolution candidate = candidate_solution.current + candidate_solution.velocity*candidate_solution.direction*(candidate_solution.current - mPopulation[pop_iter].current);
						candidate = mUpper_Bound.min(mLower_Bound.max(candidate));//also ensure the bounds

						candidate_solution.metric_calculator->Reset();
						const double fitness = mFitness.Calculate_Fitness(candidate, candidate_solution.metric_calculator);
						if (fitness < candidate_solution.next_fitness) {
							candidate_solution.next = candidate;
							candidate_solution.next_fitness = fitness;							
						}
					}

					if (candidate_solution.next_fitness >= candidate_solution.current_fitness) {
						//we have not improved, let's try a different search direction

						//let's try the very next binary-encoded integer as the vector, until we flip over to zero
						//zero means a desired, one-generation-only stagnation
						//with this, we do something like mutation and crosbreeding combined

						bool carry = false;
						for (size_t i = 0; i < solution_size; i++) {
							
							auto add_one = [this](double &number)->bool {	//returns carry
								bool carry = false;

								if (number < 0.0) number = 0.0;
								else if (number == 0.0) number = mInitial_Velocity;
								else if (number > 0.0) {
									number = -mInitial_Velocity;
									carry = true;
								}

								return carry;
							};


							TSolution &direction = candidate_solution.direction;
							if (carry) add_one(direction[i]);	//adding carry from the previous grade/position
							carry = add_one(direction[i]);		//adding 1 to the current grade/position
						}

						candidate_solution.velocity = mInitial_Velocity;
					}
					else
						candidate_solution.velocity *= mVelocity_Increase;
				}
			});


			//3. copy the current results
			//we must make the copy to avoid non-determinism that could arise by having current solution only 
			for (auto &solution : mPopulation) {
				solution.current = solution.next;
				solution.current_fitness = solution.next_fitness;
			}

		} //while in the progress

		//find the best result and return it
		const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const Deterministic_Evolution_internal::TCandidate<TSolution> &a, const Deterministic_Evolution_internal::TCandidate<TSolution> &b) {return a.current_fitness < b.current_fitness; });
		return result->current;
	
	}
};
