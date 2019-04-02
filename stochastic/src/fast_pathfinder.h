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


#include <random>
#include <tbb/parallel_for.h>

#include <math.h>

#include "../../../common/rtl/AlignmentAllocator.h"


namespace fast_pathfinder_internal {
	template <typename TUsed_Solution>
	struct TCandidate {
		TUsed_Solution current, next;
		size_t leader_index;
		double current_fitness, next_fitness;

		TUsed_Solution direction;

		//the following members safe to either guarantee thread safety and to avoid memory allocations
		TUsed_Solution quadratic;
		Eigen::Matrix3d A;
		Eigen::Vector3d b, coeff;

		std::mt19937_64 random_generator;
		std::uniform_real_distribution<double> uniform_distribution{ -1.0, 1.0 };
	};
}

#undef min

template <typename TUsed_Solution>// size_t mPopulation_Size = 15, size_t mGeneration_Count = 200000,	
class CFast_Pathfinder : public virtual CAligned<AVX2Alignment> {
protected:
	solver::TSolver_Setup mSetup;
protected:
	double mAngle_Stepping = 0.45;

	void Fill_Population_From_Candidates(const TAligned_Solution_Vector<TUsed_Solution> &candidates) {
		std::vector<double> fitness;
		std::vector<size_t> indexes;

		for (size_t i = 0; i < candidates.size(); i++) {
			fitness.push_back(mSetup.objective(mSetup.data, candidates[i].data()));
			indexes.push_back(i);
		}

		std::partial_sort(indexes.begin(), indexes.begin() + mSetup.population_size, indexes.end(), [&fitness](const size_t a, const size_t b) {return fitness[a] < fitness[b]; });

		for (size_t i = 0; i < mSetup.population_size; i++)
			mPopulation[i].current = candidates[indexes[i]];
	}

	TAligned_Solution_Vector<TUsed_Solution> Generate_Spheric_Population(const TUsed_Solution &unit_offset) {

		const auto bounds_range = mUpper_Bound - mLower_Bound;

		TAligned_Solution_Vector<TUsed_Solution> solutions;		

		const size_t step_count = std::max(mSetup.population_size, static_cast<size_t>(655));	//minimum number at which the default stepping angle is slightly irregular
		const double diameter_stepping = 1.0 / static_cast<double>(step_count);	//it overlaps due to 1.0 and that we start from the center - cand_val = 0.5 + ...

		double diameter_stepped = 0.0;
		double angle_stepped = 0.0;

		for (size_t i = 0; i < step_count; i++) {

			TUsed_Solution tmp;
			tmp.resize(Eigen::NoChange, bounds_range.cols());


			for (auto j = 0; j < bounds_range.cols(); j++) {

				//const double di = static_cast<double>(i)*stepped;
				double cand_val = unit_offset[j];


				if ((j & 1) == 0) cand_val += diameter_stepped * sin(angle_stepped);
					else cand_val += diameter_stepped * cos(angle_stepped);


				while (cand_val < 0.0)
					cand_val += 1.0;
				while (cand_val > 1.0)
					cand_val -= 1.0;

				tmp[j] = cand_val;
			}

			diameter_stepped += diameter_stepping;
			angle_stepped += mAngle_Stepping;


			tmp = mLower_Bound + tmp.cwiseProduct(bounds_range);
			solutions.push_back(tmp);
		}

		return solutions;
	}

protected:
	TAligned_Solution_Vector<fast_pathfinder_internal::TCandidate<TUsed_Solution>> mPopulation;

	TUsed_Solution Calculate_Quadratic_Candidate_From_Population() {

		TUsed_Solution quadratic;
		quadratic.resize(Eigen::NoChange, mSetup.problem_size);

		Eigen::Matrix<double, Eigen::Dynamic, 3> A;
		Eigen::Matrix<double, Eigen::Dynamic, 1> b;
		A.resize(mPopulation.size(), Eigen::NoChange);
		A.setConstant(1.0);
		b.resize(mPopulation.size(), Eigen::NoChange);


		for (size_t i = 0; i < mPopulation.size(); i++)
			b(i) = mPopulation[i].next_fitness;

		for (size_t j = 0; j < mSetup.problem_size; j++) {
			for (size_t i = 0; i < mPopulation.size(); i++) {
				const double x = mPopulation[i].next[j];
				A(i, 0) = x * x; A(i, 1) = x;
			}

			const Eigen::Vector3d coeff = A.jacobiSvd(Eigen::ComputeFullV | Eigen::ComputeFullU).solve(b);

			//now, we have calculated a*x*x + b*x +c = y,  hence first derivative is
			//2a*x +b = 0 => -b/2a gives the extreme position - the x^2 member must be positive in order for the polynomial to has a minimum
			if (coeff[0] > 0.0) quadratic[j] = -coeff[1] / (2.0*coeff[0]);				//if not, we preserve the better parameters only																
					//else quadratic[j] = mLower_Bound[j] + uniform_distribution(MT_sequence)*bounds_range[j];
				else quadratic[j] = std::numeric_limits<double>::quiet_NaN();
		}

		quadratic = mUpper_Bound.min(mLower_Bound.max(quadratic));

		return quadratic;
	}
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;

	bool Evolve_Solution(fast_pathfinder_internal::TCandidate<TUsed_Solution> &candidate_solution, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &leading_solution) {
		//returns true, if found_solution has better found fitness than the current fitness of the candidate_solution

		bool improved = false;

		candidate_solution.next_fitness = candidate_solution.current_fitness;

		auto check_solution = [&](TUsed_Solution &solution) {
			solution = mUpper_Bound.min(mLower_Bound.max(solution));
			const double fitness = mSetup.objective(mSetup.data, solution.data());

			if (fitness < candidate_solution.next_fitness) {
				candidate_solution.next = solution;
				candidate_solution.next_fitness = fitness;

				improved = true;
			}

			return fitness;
		};


		auto &A = candidate_solution.A;
		auto &b = candidate_solution.b;
		auto &coeff = candidate_solution.coeff;

		auto &quadratic_candidate = candidate_solution.quadratic;

		quadratic_candidate = candidate_solution.current + candidate_solution.direction*(candidate_solution.current - leading_solution.current);
	
		b(0) = candidate_solution.current_fitness;
		b(1) = check_solution(quadratic_candidate);
		b(2) = leading_solution.current_fitness;

		for (size_t i = 0; i < mSetup.problem_size; i++) {
			const double &x1 = candidate_solution.current[i];
			const double &x2 = quadratic_candidate[i];
			const double &x3 = leading_solution.current[i];

			A(0, 0) = x1 * x1; A(0, 1) = x1;
			A(1, 0) = x2 * x2; A(1, 1) = x2;
			A(2, 0) = x3 * x3; A(2, 1) = x3;			

			//coeff = (A.transpose() * A).ldlt().solve(A.transpose() * b);//A.jacobiSvd(Eigen::ComputeFullV | Eigen::ComputeFullU).solve(b);
			coeff = A.jacobiSvd(Eigen::ComputeFullV | Eigen::ComputeFullU).solve(b);			

			//now, we have calculated a*x*x + b*x +c = y,  hence first derivative is
			//2a*x +b = 0 => -b/2a gives the extreme position - the x^2 member must be positive in order for the polynomial to has a minimum
			if (coeff[0] > 0.0) quadratic_candidate[i] = -coeff[1] / (2.0*coeff[0]);				//if not, we preserve the better parameters only												
			else {
				if (!improved) {
					candidate_solution.direction[i] = candidate_solution.uniform_distribution(candidate_solution.random_generator);

					if (coeff[0] < 0.0) {
						//quadratic_candidate[i] = 0.5*(candidate_solution.current[i] + (-coeff[1] / (2.0*coeff[0])));

						const double x_candidate = 0.5*(candidate_solution.current[i] + (-coeff[1] / (2.0*coeff[0])));
						const double x_leading = 0.5*(leading_solution.current[i] + (-coeff[1] / (2.0*coeff[0])));

						const double y_candidate = x_candidate * x_candidate*coeff[0] + x_candidate * coeff[1] + coeff[2];
						const double y_leading = x_leading * x_leading*coeff[0] + x_leading * coeff[1] + coeff[2];

						quadratic_candidate[i] = y_leading < y_candidate ? x_leading : x_candidate;
					}

					else {
						const double diff = leading_solution.current[i] - candidate_solution.current[i];
						const double k = leading_solution.current_fitness < candidate_solution.current_fitness ? 0.25 : -0.25;
						quadratic_candidate[i] = candidate_solution.current[i] + diff * k;											  
					}
				}
			}
		}

		check_solution(quadratic_candidate);


		if (!improved) candidate_solution.next = candidate_solution.current;

		return improved;

	}	

public:
	CFast_Pathfinder(const solver::TSolver_Setup &setup, const double angle_stepping = 3.14*2.0 / 37.0) :
				mAngle_Stepping(angle_stepping),
				mSetup(solver::Check_Default_Parameters(setup, 100'000, 100)), 
				mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)) {
		
		mPopulation.resize(mSetup.population_size);

		//1. create the initial population
	
		//a) by creatuing deterministically generated solutions
		{
		
			const auto bounds_range = mUpper_Bound - mLower_Bound;

			TUsed_Solution unit_offset;
			unit_offset.resize(Eigen::NoChange, mSetup.problem_size);
			unit_offset.setConstant(0.5);
			
			TAligned_Solution_Vector<TUsed_Solution> solutions;
			TAligned_Solution_Vector<TUsed_Solution> best_solutions;
			solutions = Generate_Spheric_Population(unit_offset);

			//b) and appending the supplied hints
			for (size_t i = 0; i < mSetup.hint_count; i++) {
				solutions.push_back(mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size))));//also ensure the bounds
			}

			Fill_Population_From_Candidates(solutions);

			double best_fitness = std::numeric_limits<double>::max();

			for (size_t zoom_index = 0; zoom_index < 100; zoom_index++) {

				//calculate the metrics
				for (auto &solution : mPopulation) {
					solution.next_fitness = solution.current_fitness = mSetup.objective(mSetup.data, solution.current.data());
					solution.next = solution.current;
				}

				//find the best as the next offset - needs recalculation
				auto global_best = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });

				//try to improve by quadratic
				TUsed_Solution quadratic = Calculate_Quadratic_Candidate_From_Population();
				for (size_t j = 0; j < mSetup.problem_size; j++)
					if (isnan(quadratic[j])) quadratic[j] = global_best->current[j];

				const double q_f = mSetup.objective(mSetup.data, quadratic.data());
				if (q_f < global_best->current_fitness) {
					global_best->current_fitness = q_f;
					global_best->current = quadratic;
				}

				best_solutions.push_back(global_best->current);

				if (global_best->current_fitness < best_fitness) best_fitness = global_best->current_fitness;				
					else break;

				unit_offset = global_best->current - mLower_Bound;
				for (size_t j = 0; j < mSetup.problem_size; j++) {
					if (bounds_range[j] != 0.0) unit_offset[j] /= bounds_range[j];
					else unit_offset[j] = 0.0;
				}
				
				solutions = Generate_Spheric_Population(unit_offset);
				Fill_Population_From_Candidates(solutions);

			}

			//eventually, back-inject previously known best solutions
			auto global_worst = std::max_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
			while (!best_solutions.empty()) {
				const auto tmp = best_solutions.back();
				const double t_f = mSetup.objective(mSetup.data, tmp.data());
				if (t_f >= global_worst->current_fitness) break;

				best_solutions.pop_back();
				global_worst->current = tmp;
				global_worst->current_fitness = t_f;

				global_worst = std::max_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
			}
		}

	


		//2. and fill the rest
		for (auto &solution : mPopulation) {			
			//first, we need to calculate the current fitnesses
			solution.current_fitness = mSetup.objective(mSetup.data, solution.current.data());
			
			solution.leader_index = 0;	//as the population is sorted, zero is the best leader

			//the main solving algorithm expects that next contains valid values from the last run of the generation
			solution.next = solution.current;
			solution.next_fitness = solution.current_fitness;	

			solution.direction.resize(Eigen::NoChange, mSetup.problem_size);
			solution.direction.setConstant(solution.uniform_distribution.max());
			solution.quadratic.resize(Eigen::NoChange, mSetup.problem_size);
			solution.A.setConstant(1.0);
		}		
		mPopulation[0].leader_index = 1;	//so that it does not point to itself
	};

	
	
	TUsed_Solution Solve(solver::TSolver_Progress &progress) {

		progress.current_progress = 0;
		progress.max_progress = mSetup.max_generations;
				


//		std::mt19937 MT_sequence;	//to be completely deterministic in every run we used the constant, default seed
//		std::uniform_real_distribution<double> uniform_distribution(0.0, 1.0);


		const auto bounds_range = mUpper_Bound - mLower_Bound;



		while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == 0)) {

			//update the progress
			auto global_best = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
			progress.best_metric = global_best->current_fitness;			

			//2. Calculate the next vectors and their fitness 
			//In this step, current is read-only and next is write-only => no locking is needed
			//as each next will be written just once.
			//We assume that parallelization cost will get amortized
			tbb::parallel_for(tbb::blocked_range<size_t>(size_t(0), mSetup.population_size), [=](const tbb::blocked_range<size_t> &r) {


				const size_t rend = r.end();
				for (size_t iter = r.begin(); iter != rend; iter++) {
					auto &candidate_solution = mPopulation[iter];

					if (!Evolve_Solution(candidate_solution, mPopulation[candidate_solution.leader_index])) {
						
						candidate_solution.leader_index++;
						if (candidate_solution.leader_index >= mPopulation.size()) candidate_solution.leader_index = 0;

						if (!Evolve_Solution(candidate_solution, *global_best))
							for (size_t i = 0; i < mSetup.problem_size; i++)
								candidate_solution.direction[i] = candidate_solution.uniform_distribution(candidate_solution.random_generator);																	
					}
				}
			});


			auto global_worst = std::max_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });

			bool reached = true;
			for (size_t i = 0; i < mSetup.problem_size; i++) {
				if (global_best->next[i] != global_worst->next[i]) {
					reached = false;
					break;
				}

			}

			if (reached) {
				global_best->current = global_best->next;
				break;
			}

			TUsed_Solution quadratic = Calculate_Quadratic_Candidate_From_Population();
			for (size_t j = 0; j < mSetup.problem_size; j++)
				//				if (isnan(quadratic[j])) quadratic[j] = mLower_Bound[j] + uniform_distribution(MT_sequence)*bounds_range[j];
				if (isnan(quadratic[j])) quadratic[j] = global_best->current[j]; //moving one step back to previous generation
			
			const double q_f = mSetup.objective(mSetup.data, quadratic.data());
			
			if (q_f < global_worst->next_fitness) {
				global_worst->next = quadratic;
				global_worst->next_fitness = q_f;
			}


			
			//3. copy the current results
			//we must make the copy to avoid non-determinism that could arise by having current solution only 
			for (auto &solution : mPopulation) {
				solution.current = solution.next;
				solution.current_fitness = solution.next_fitness;
			}

		} //while in the progress

		//find the best result and return it
		const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
		return result->current;
	
	}
};