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


#include <random>
#include <tbb/parallel_for.h>

#include "third party/little-big-int.hpp"
#include <math.h>

namespace pathfinder_internal {
	template <typename TUsed_Solution>
	struct TCandidate {
		TUsed_Solution current, next;
		size_t direction_index;
		double velocity;
		double current_fitness, next_fitness;
	};
}

#undef min

template <typename TUsed_Solution, bool mUse_LD_Directions, bool mUse_LD_Population>// size_t mPopulation_Size = 15, size_t mGeneration_Count = 200000,	
class CPathfinder {
protected:
	solver::TSolver_Setup mSetup;
protected:
	const double mInitial_Velocity = 1.0 / 3.0;
	const double mVelocity_Increase = 1.05;

	TAligned_Solution_Vector<TUsed_Solution> mDirections;
	void Generate_Directions() {

		const size_t solution_size = mLower_Bound.cols();
		const size_t combination_count = static_cast<size_t>(pow(3.0, static_cast<double>(solution_size)));			

		//1. generate the deterministic, uniform direction vectors

		//let's try the very next binary-encoded integer as the vector, until we flip over to zero
		//zero means a desired, one-generation-only stagnation
		//with this, we do something like mutation and crosbreeding combined

		TUsed_Solution direction;
		direction.resize(Eigen::NoChange, solution_size);
		direction.setConstant(1.0);		//init to max so it overflow to min as the first pushed value

		for (size_t combination = 0; combination < combination_count; combination++) {
			auto add_one = [this](double &number)->bool {	//returns carry
				bool carry = false;

				if (number < 0.0) number = 0.0;
				else if (number == 0.0) number = 1.0;
				else if (number > 0.0) {
					number = -1.0;
					carry = true;
				}

				return carry;
			};
			
			bool carry = add_one(direction[0]);
			for (size_t i = 1; carry && (i < solution_size); i++) {			
				carry = add_one(direction[i]);				
			}

			mDirections.push_back(direction);
		}


		//2. generate pseudo random directions
		if (mUse_LD_Directions) {
			std::mt19937 MT_sequence;	//to be completely deterministic in every run we used the constant, default seed
			std::uniform_real_distribution<double> uniform_distribution(-1.0, 1.0);
			for (size_t i = 0; i < combination_count * 3; i++) {
				for (auto j = 0; j < solution_size; j++)
					direction[j] = uniform_distribution(MT_sequence);

				mDirections.push_back(direction);
			}
		}
	}

	void Generate_LD_Population(const size_t initialized_count) {
		std::mt19937 MT_sequence;	//to be completely deterministic in every run we used the constant, default seed
		std::uniform_real_distribution<double> uniform_distribution(0.0, 1.0);
		CHalton_Device halton;

		const auto bounds_range = mUpper_Bound - mLower_Bound;

		TAligned_Solution_Vector<TUsed_Solution> solutions;

		std::vector<double> fitness;
		std::vector<size_t> fi;
		for (size_t i = 0; i < mSetup.population_size*5; i++) {
			TUsed_Solution tmp;
			tmp.resize(Eigen::NoChange, bounds_range.cols());

			for (auto j = 0; j < bounds_range.cols(); j++)
				tmp[j] = uniform_distribution(MT_sequence);

			solutions.push_back(tmp);
			fitness.push_back(mSetup.objective(mSetup.data, solutions[i].data()));
			fi.push_back(i);
		}


		std::sort(fi.begin(), fi.end(), [&fitness](const size_t a, const size_t b) {return fitness[a] < fitness[b]; });

		for (size_t i = initialized_count; i < mSetup.population_size; i++)
			mPopulation[i].current = solutions[fi[i- initialized_count]];



/*		for (size_t i = initialized_count; i < mSetup.population_size; i++) {
			TUsed_Solution tmp;

			// this helps when we use generic solution vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
			// optimized away at compile time
			tmp.resize(Eigen::NoChange, bounds_range.cols());

			for (auto j = 0; j < bounds_range.cols(); j++)
				tmp[j] = uniform_distribution(MT_sequence);

			mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
		}
*/
	}

	void discretize(const size_t index, TUsed_Solution &sample, TAligned_Solution_Vector<TUsed_Solution> &solutions, std::vector<double> &steps) {
		const auto bounds_range = mUpper_Bound - mLower_Bound;
		
		

		for (const auto &step : steps) {
			TUsed_Solution tmp = sample;
			tmp[index] = step;

			if (index > 0) discretize(index - 1, tmp, solutions, steps);
				else solutions.push_back(mLower_Bound + tmp.cwiseProduct(bounds_range));
		}
	};

	void Generate_Deterministic_Population(const size_t initialized_count) {
		
		const double stepping = 0.001;//1.0/static_cast<double>(mSetup.population_size);
		double stepped = stepping*0.5;
		std::vector<double> steps; 
		//for (size_t i = 0; i < steps.size(); i++)
		while (stepped < 1.0) {
			steps.push_back(stepped);
			stepped += stepping;
		}

		TAligned_Solution_Vector<TUsed_Solution> solutions;
		TUsed_Solution tmp;
		tmp.resize(Eigen::NoChange, mUpper_Bound.cols());
		discretize(mUpper_Bound.cols() - 1, tmp, solutions, steps);
		

		std::vector<double> fitness;
		std::vector<size_t> fi;
		for (size_t i = 0; i < solutions.size(); i++) {
			fitness.push_back(mSetup.objective(mSetup.data, solutions[i].data()));
			fi.push_back(i);
		}
		

		std::sort(fi.begin(), fi.end(), [&fitness](const size_t a, const size_t b) {return fitness[a] < fitness[b]; });

		for (size_t i = initialized_count; i < mSetup.population_size; i++)
			mPopulation[i].current = solutions[fi[i- initialized_count]];

		return;
		
/*

		const auto bounds_range = mUpper_Bound - mLower_Bound;

		//const double step = 3.14*0.5*0.75 / static_cast<double>(mSetup.population_size - initialized_count);
		const double step = 0.6;

		double stepped = 0.0;

		for (size_t i = initialized_count; i < mSetup.population_size; i++) {

			TUsed_Solution tmp;
			tmp.resize(Eigen::NoChange, bounds_range.cols());

			
			for (auto j = 0; j < bounds_range.cols(); j++) {

				const double di = static_cast<double>(i)*stepped;
				double cand_val = 0.5 + 0.25 * sin(di);
				while (cand_val < 0.0)
					cand_val += 1.0;
				while (cand_val > 1.0)
					cand_val -= 1.0;

				tmp[j] = cand_val;

				stepped += step;
			}
			
			mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
		}


		return;


		double N = static_cast<double>(mSetup.population_size - initialized_count);	//how many points we have to generate
		const BigUint axis_intersections{ static_cast<BigUint::Block>(std::max(2.0, ceil(pow(N, 1.0 / static_cast<double>(mSetup.problem_size))))) };					//at how many places we have to intersect each axis - at leas two are needed		
		//Hence the ideal number of points would be the number of axis intersections to the power of problem_size.
		//While e.g, problem size 2 with 3 intersections would create a nice rectangle with 9 points, it is not feasible e.g., for a problem size like 100. Therefore, we must use a big-integer library to do the trick.		 

		BigUint ideal_point_count{ 1};
		//do the power
		for (size_t i = 0; i < mSetup.problem_size; i++)
			ideal_point_count *= axis_intersections;

		const BigUint real_point_count{ static_cast<BigUint::Block>(N) };
		const BigUint stepping = ideal_point_count / real_point_count;
		BigUint coord{ 0 };

		const double axis_stepping = 1.0 / static_cast<double>(axis_intersections.getSimple()+1);
		const double axis_skew = 1.0 / (axis_stepping*0.8);
		size_t index = initialized_count;		

		std::uniform_real_distribution<double> uniform_distribution(0.0, 1.0);
		double last_skew = 1.0;
		while (coord < real_point_count) {
			TUsed_Solution tmp;
			tmp.resize(Eigen::NoChange, bounds_range.cols());

			BigUint tmp_coord = coord;
			for (auto j = 0; j < bounds_range.cols(); j++) {
				const size_t axis_coord = (tmp_coord % axis_intersections).getSimple();
				tmp_coord /= axis_intersections;

				tmp[j] = trunc(axis_stepping * static_cast<double>(axis_coord + 1) * last_skew);
				
				last_skew *= 1.033;
			}				
			

			mPopulation[index].current = mLower_Bound + tmp.cwiseProduct(bounds_range);
			index++;

			coord += stepping;
		}

		assert(index == mPopulation.size());
		*/
	}

protected:
	TAligned_Solution_Vector<pathfinder_internal::TCandidate<TUsed_Solution>> mPopulation;
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
public:
	CPathfinder(const solver::TSolver_Setup &setup) : 
				mSetup(solver::Check_Default_Parameters(setup, 20'000, 15)), 
				mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)) {
		
		mPopulation.resize(mSetup.population_size);

		//1. create the initial population
		const size_t initialized_count = std::min(mSetup.population_size / 2, mSetup.hint_count);
		//a) by storing suggested params
		for (size_t i = 0; i < initialized_count; i++) {
			mPopulation[i].current = mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size)));//also ensure the bounds
		}

		//b) by complementing it with randomly generated numbers
		if (mUse_LD_Population) Generate_LD_Population(initialized_count);
			else Generate_Deterministic_Population(initialized_count);

		//2. and calculate the metrics
		for (auto &solution : mPopulation) {
			solution.current_fitness = mSetup.objective(mSetup.data, solution.current.data());
			
			//the main solving algorithm expects that next contains valid values from the last run of the generation
			solution.next = solution.current;
			solution.next_fitness = solution.current_fitness;
			
			solution.direction_index = 0;
			solution.velocity = mInitial_Velocity;
		}		

		Generate_Directions();
	};

	TUsed_Solution Solve(solver::TSolver_Progress &progress) {
		progress.current_progress = 0;
		progress.max_progress = mSetup.max_generations;

		const size_t solution_size = mPopulation[0].current.cols();
		
		while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == 0)) {

			//update the progress
			const auto global_best = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const pathfinder_internal::TCandidate<TUsed_Solution> &a, const pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
			progress.best_metric = global_best->current_fitness;			

			//2. Calculate the next vectors and their fitness 
			//In this step, current is read-only and next is write-only => no locking is needed
			//as each next will be written just once.
			//We assume that parallelization cost will get amortized
			tbb::parallel_for(tbb::blocked_range<size_t>(size_t(0), mSetup.population_size), [=](const tbb::blocked_range<size_t> &r) {


				const size_t rend = r.end();
				for (size_t iter = r.begin(); iter != rend; iter++) {


					auto &candidate_solution = mPopulation[iter];

					//try to advance the current solution in the direction of the other solutions

					for (size_t pop_iter = 0; pop_iter < mSetup.population_size; pop_iter++) {						
						TUsed_Solution candidate = candidate_solution.current + candidate_solution.velocity*mDirections[candidate_solution.direction_index]*(candidate_solution.current - mPopulation[pop_iter].current);
						candidate = mUpper_Bound.min(mLower_Bound.max(candidate));//also ensure the bounds
						
						const double fitness = mSetup.objective(mSetup.data, candidate.data());
						if (fitness < candidate_solution.next_fitness) {
							candidate_solution.next = candidate;
							candidate_solution.next_fitness = fitness;
						}
					}

					if (candidate_solution.next_fitness >= candidate_solution.current_fitness) {
						//we have not improved, let's try a different search direction						

						candidate_solution.direction_index++;
						if (candidate_solution.direction_index >= mDirections.size())
							candidate_solution.direction_index = 0;

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
		const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const pathfinder_internal::TCandidate<TUsed_Solution> &a, const pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
		return result->current;
	
	}
};
