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

#include <math.h>

#include <unordered_set>

namespace fast_pathfinder_internal {
	template <typename TUsed_Solution>
	struct TCandidate {
		TUsed_Solution current, next;
		size_t population_index;
		double current_fitness, next_fitness;
	};
}

#undef min

template <typename TUsed_Solution, bool mUse_LD_Directions, bool mUse_LD_Population>// size_t mPopulation_Size = 15, size_t mGeneration_Count = 200000,	
class CFast_Pathfinder {
protected:
	solver::TSolver_Setup mSetup;
protected:
	double mAngle_Stepping = 0.45;

	const size_t mMax_Deterministic_Directions_By_Problem_Size = 12;	//half a million of deterministic directions
	TAligned_Solution_Vector<TUsed_Solution> mDirections;
	void Generate_Directions() {

		const size_t solution_size = mLower_Bound.cols();
		const size_t combination_count = static_cast<size_t>(pow(3.0, static_cast<double>(std::min(solution_size, mMax_Deterministic_Directions_By_Problem_Size))));
		const bool override_directions_by_problem_size = solution_size > mMax_Deterministic_Directions_By_Problem_Size;
			//Actually, if we would not cache mDirections, but calculate them on the fly, we could stay deterministic. However, the number of directions would be so
			//big that it would make the calculation unfeasible.

		//1. generate the deterministic, uniform direction vectors

		//let's try the very next binary-encoded integer as the vector, until we flip over to zero
		//zero means a desired, one-generation-only stagnation
		//with this, we do something like mutation and crosbreeding combined

		TUsed_Solution direction;
		direction.resize(Eigen::NoChange, solution_size);
		direction.setConstant(1.0);		//init to max so it overflow to min as the first pushed value

		if (!override_directions_by_problem_size) {
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
		}


		//2. generate pseudo random directions
		if (mUse_LD_Directions || override_directions_by_problem_size) {
			std::mt19937 MT_sequence;	//to be completely deterministic in every run we used the constant, default seed
			std::uniform_real_distribution<double> uniform_distribution(-1.0, 1.0);
			for (size_t i = 0; i < combination_count * 3; i++) {
				for (auto j = 0; j < solution_size; j++)
					direction[j] = uniform_distribution(MT_sequence);

				mDirections.push_back(direction);
			}
		}
	}

	void Generate_Triangle_Directions() {
		TUsed_Solution right_triangle, left_triangle, saw, mid_point_triangle, mirrored_mid_point_triangle;
		right_triangle.resize(Eigen::NoChange, mSetup.problem_size);
		left_triangle.resize(Eigen::NoChange, mSetup.problem_size);
		mid_point_triangle.resize(Eigen::NoChange, mSetup.problem_size);
		mirrored_mid_point_triangle.resize(Eigen::NoChange, mSetup.problem_size);
		saw.resize(Eigen::NoChange, mSetup.problem_size);

		//push the basic directions
		saw.setConstant(1.0);
		mDirections.push_back(saw);
		mDirections.push_back(-saw);

		if (mSetup.problem_size < 2) return;

		const double k = 1.0/ static_cast<double>(mSetup.problem_size-1);
		for (size_t i = 0; i < mSetup.problem_size; i++)
			right_triangle[i] = static_cast<double>(i)*k;
		left_triangle = 1.0 - right_triangle;
				
		const size_t mid_index = mSetup.problem_size / 2;
		for (size_t i = 0; i < mid_index; i++)
			mid_point_triangle[i] = 2.0*static_cast<double>(i)*k;
		for (size_t i = mid_index; i < mSetup.problem_size; i++)
			mid_point_triangle[i] = 2.0-2.0*static_cast<double>(i)*k;
		
		mirrored_mid_point_triangle = 1.0 - mid_point_triangle;

		saw = mid_point_triangle.max(left_triangle.max(right_triangle));

		//push the shapes above the zero
		mDirections.push_back(right_triangle);
		mDirections.push_back(left_triangle);
		mDirections.push_back(mid_point_triangle);
		mDirections.push_back(mirrored_mid_point_triangle);
		mDirections.push_back(saw);
		mDirections.push_back(1.0-saw);

		//mirror them to the negative axis
		mDirections.push_back(-right_triangle);
		mDirections.push_back(-left_triangle);
		mDirections.push_back(-mid_point_triangle);
		mDirections.push_back(-mirrored_mid_point_triangle);
		mDirections.push_back(-saw);
		mDirections.push_back(saw - 1.0);

		//and strech the combinations across the entire boundaries
		mDirections.push_back(2.0*right_triangle - 1.0);
		mDirections.push_back(2.0*left_triangle - 1.0);
		mDirections.push_back(2.0*mid_point_triangle - 1.0);
		mDirections.push_back(2.0*mirrored_mid_point_triangle-1.0);
		mDirections.push_back(2.0*saw - 1.0);
		mDirections.push_back(1.0 - 2.0*saw);
		
		mDirections.push_back(-2.0*right_triangle + 1.0);
		mDirections.push_back(-2.0*left_triangle + 1.0);
		mDirections.push_back(-2.0*mid_point_triangle + 1.0);
		mDirections.push_back(-2.0*mirrored_mid_point_triangle + 1.0);
		//stretched saw is already mirrored

		//at this point, some duplicities occurs for dimensions 2 and 3, but it is not necessary to remove them => so, let's rather keep the code shorter

/*
		for (const TUsed_Solution &x : mDirections) {
			for (size_t i = 0; i < mSetup.problem_size; i++)
				std::cout << x[i] << " ";
			std::cout << std::endl;
		}

		std::cout << std::endl;
*/

	}

	void Fill_Population_From_Candidates(const size_t initialized_count, const TAligned_Solution_Vector<TUsed_Solution> &candidates) {
		std::vector<double> fitness;
		std::vector<size_t> indexes;

		for (size_t i = 0; i < candidates.size(); i++) {
			fitness.push_back(mSetup.objective(mSetup.data, candidates[i].data()));
			indexes.push_back(i);
		}

		std::partial_sort(indexes.begin(), indexes.begin() + mSetup.population_size - initialized_count, indexes.end(), [&fitness](const size_t a, const size_t b) {return fitness[a] < fitness[b]; });

		for (size_t i = initialized_count; i < mSetup.population_size; i++)
			mPopulation[i].current = candidates[indexes[i - initialized_count]];
	}

	TAligned_Solution_Vector<TUsed_Solution> Generate_LD_Population() {
		std::mt19937 MT_sequence;	//to be completely deterministic in every run we used the constant, default seed
		std::uniform_real_distribution<double> uniform_distribution(0.0, 1.0);

		const auto bounds_range = mUpper_Bound - mLower_Bound;

		TAligned_Solution_Vector<TUsed_Solution> solutions;
		
		for (size_t i = 0; i < mSetup.population_size*5; i++) {
			TUsed_Solution tmp;
			tmp.resize(Eigen::NoChange, bounds_range.cols());

			for (auto j = 0; j < bounds_range.cols(); j++)
				tmp[j] = uniform_distribution(MT_sequence);

			solutions.push_back(tmp);
		}

		return solutions;
	}

	TAligned_Solution_Vector<TUsed_Solution> Generate_Spheric_Population() {

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
				double cand_val = 0.5;


				if ((j & 1) == 0) cand_val += diameter_stepped * sin(angle_stepped);
					else cand_val += diameter_stepped * cos(angle_stepped);


				while (cand_val < 0.0)
					cand_val += 1.0;
				while (cand_val > 1.0)
					cand_val -= 1.0;

				tmp[j] = cand_val;

				diameter_stepped += diameter_stepping;
				angle_stepped += mAngle_Stepping;
			}

			tmp = mLower_Bound + tmp.cwiseProduct(bounds_range);
			solutions.push_back(tmp);
		}

		return solutions;
	}
protected:
	TAligned_Solution_Vector<fast_pathfinder_internal::TCandidate<TUsed_Solution>> mPopulation;
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
public:
	CFast_Pathfinder(const solver::TSolver_Setup &setup, const double angle_stepping = 3.14*2.0 / 37.0) :
				mAngle_Stepping(angle_stepping),
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
		{
			TAligned_Solution_Vector<TUsed_Solution> solutions;
			if (mUse_LD_Population) solutions = Generate_LD_Population();
				else solutions = Generate_Spheric_Population();
			Fill_Population_From_Candidates(initialized_count, solutions);
		}

		//2. and calculate the metrics
		for (auto &solution : mPopulation) {
			solution.current_fitness = mSetup.objective(mSetup.data, solution.current.data());
			
			//the main solving algorithm expects that next contains valid values from the last run of the generation
			solution.next = solution.current;
			solution.next_fitness = solution.current_fitness;		
		}		

		if (mUse_LD_Directions) Generate_Directions();
			else Generate_Triangle_Directions();
	};


	void Find_Best_Leader(fast_pathfinder_internal::TCandidate<TUsed_Solution> &candidate_solution) {
		//also updates next solution and fitness if it finds a better one

		for (size_t i = 0; i < mPopulation.size(); i++) 
			if (Eval_Solution(candidate_solution, mPopulation[i], candidate_solution.next, candidate_solution.next_fitness)) {
				candidate_solution.population_index = i;
			}
	}

	bool Eval_Solution(const fast_pathfinder_internal::TCandidate<TUsed_Solution> &candidate_solution, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &leading_solution,
		TUsed_Solution &found_solution, double &found_fitness) {
		//returns true, if found_solution has better found fitness than the current fitness of the candidate_solution

		bool improved = false;
		
		found_fitness = candidate_solution.current_fitness;

		auto check_solution = [&](TUsed_Solution &solution) {
			solution = mUpper_Bound.min(mLower_Bound.max(solution));
			const double fitness = mSetup.objective(mSetup.data, solution.data());

			if (fitness < found_fitness) {
				found_solution = solution;
				found_fitness = fitness;

				improved = true;
			}

			return fitness;
		};

		const TUsed_Solution diff = candidate_solution.current - leading_solution.current;
		TUsed_Solution quadratic_candidate = candidate_solution.current;
		quadratic_candidate.resize(Eigen::NoChange, mSetup.problem_size);

		Eigen::Matrix3d A = Eigen::Matrix3d::Ones();
		Eigen::Vector3d b;

		b(0) = candidate_solution.current_fitness;
		b(2) = leading_solution.current_fitness;

		for (size_t i = 0; i < mDirections.size(); i++) {
			TUsed_Solution sample = candidate_solution.current + diff*mDirections[i];
			const double sample_fitness = check_solution(sample);			
			

			for (size_t i = 0; i < mSetup.problem_size; i++) {			
				const double &x1 = candidate_solution.current[i];

				const double &x2 = sample[i];
				const double &y2 = sample_fitness;

				const double &x3 = leading_solution.current[i];

				A(0, 0) = x1 * x1; A(0, 1) = x1;
				A(1, 0) = x2 * x2; A(1, 1) = x2; b(1) = y2;
				A(2, 0) = x3 * x3; A(2, 1) = x3;
				
				const Eigen::Vector3d coeff = A.jacobiSvd(Eigen::ComputeFullV | Eigen::ComputeFullU).solve(b);

				//now, we have calculated a*x*x + b*x +c = y,  hence first derivative is
				//2a*x +b = 0 => -b/2a gives the extreme position - the x^2 member must be positive in order for the polynomial to has a minimum
				if (coeff[0] > 0.0) quadratic_candidate[i] = -coeff[1] / (2.0*coeff[0]);				//if not, we preserve the better parameters only												
			}
			
			check_solution(quadratic_candidate);

			//eventually, try to replace solution elements one by one
			for (size_t j = 0; j < mSetup.problem_size; j++) {
				TUsed_Solution x = found_solution;
				x[j] = quadratic_candidate[j];
				check_solution(x);				
			}
		
		}

		return improved;

	}

	TUsed_Solution Solve(solver::TSolver_Progress &progress) {
		
		for (auto &solution : mPopulation) {
			Find_Best_Leader(solution);
			solution.current = solution.next;
			solution.current_fitness = solution.next_fitness;
		}
			
	

		progress.current_progress = 0;
		progress.max_progress = mSetup.max_generations;
		
		
		while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == 0)) {

			//update the progress
			const auto global_best = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
			progress.best_metric = global_best->current_fitness;			

			//2. Calculate the next vectors and their fitness 
			//In this step, current is read-only and next is write-only => no locking is needed
			//as each next will be written just once.
			//We assume that parallelization cost will get amortized
			tbb::parallel_for(tbb::blocked_range<size_t>(size_t(0), mSetup.population_size), [=](const tbb::blocked_range<size_t> &r) {


				const size_t rend = r.end();
				for (size_t iter = r.begin(); iter != rend; iter++) {
					auto &candidate_solution = mPopulation[iter];

					if (!Eval_Solution(candidate_solution, mPopulation[candidate_solution.population_index], candidate_solution.next, candidate_solution.next_fitness)) {
						Find_Best_Leader(candidate_solution);
						//candidate_solution.population_index++;
						//if (candidate_solution.population_index >= mPopulation.size()) candidate_solution.population_index = 0;
					}
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
		const auto result = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
		return result->current;
	
	}
};
