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


#include <math.h>

template <typename TUsed_Solution>
class CLandscape_Pathfinder {
protected:
	struct TCandidate_Data {
		TUsed_Solution current, next;
		double current_fitness, next_fitness = std::numeric_limits<double>::quiet_NaN();
	};
	
	using TCandidate = std::unique_ptr<TCandidate_Data>;	//unique_ptr to allow fast sorting	
	using TPopulation = std::vector<TCandidate>;

	static bool Compare_Current_Fitness(const TCandidate &a, const TCandidate &b) {
		return a->current_fitness < b->current_fitness; 
	}

protected:
	solver::TSolver_Setup mSetup;
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
	TPopulation mPopulation;		
protected:
	double mAngle_Stepping = 0.45;
	
	
	TPopulation Generate_Spheric_Population(const TUsed_Solution &unit_offset) {

		TPopulation solutions;

		//1. generate the coordinates
		const auto bounds_range = mUpper_Bound - mLower_Bound;
		const size_t step_count = std::max(mSetup.population_size, static_cast<size_t>(655));	//minimum number at which the default stepping angle is slightly irregular
		const double diameter_stepping = 1.0 / static_cast<double>(step_count);	//it overlaps due to 1.0 and that we start from the center - cand_val = 0.5 + ...

		double diameter_stepped = 0.0;
		double angle_stepped = 0.0;

		for (size_t i = 0; i < step_count; i++) {

			TCandidate candidate = std::make_unique<TCandidate_Data>();
			candidate->current.resize(Eigen::NoChange, bounds_range.cols());


			for (auto j = 0; j < bounds_range.cols(); j++) {

				double cand_val = unit_offset[j];


				if ((j & 1) == 0) cand_val += diameter_stepped * sin(angle_stepped);
				else cand_val += diameter_stepped * cos(angle_stepped);


				while (cand_val < 0.0)
					cand_val += 1.0;
				while (cand_val > 1.0)
					cand_val -= 1.0;

				candidate->current[j] = cand_val;
			}

			diameter_stepped += diameter_stepping;
			angle_stepped += mAngle_Stepping;


			candidate->current = mLower_Bound + candidate->current.cwiseProduct(bounds_range);
			candidate->next = candidate->current;
			solutions.push_back(std::move(candidate));
		}

		//2. calculate their fitness values
		std::for_each(std::execution::par_unseq, solutions.begin(), solutions.end(), [this](TCandidate &candidate) {
			candidate->current_fitness = candidate->next_fitness = mSetup.objective(mSetup.data, candidate->current.data());
		});

		return solutions;
	}


	TUsed_Solution Solution_To_Unit_Offset(const TUsed_Solution &solution) {
		const auto bounds_range = mUpper_Bound - mLower_Bound;
		TUsed_Solution unit_offset = solution - mLower_Bound;
		for (size_t j = 0; j < mSetup.problem_size; j++) {
			if (bounds_range[j] != 0.0) unit_offset[j] /= bounds_range[j];
			else unit_offset[j] = 0.0;
		}

		return unit_offset;
	}
protected:

	TCandidate Raw_To_Candidate_No_Fitness(const double* raw) {
		TCandidate candidate = std::make_unique<TCandidate_Data>();
		candidate->current = candidate->next = mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(raw, mSetup.problem_size)));//also ensure the bounds
		return candidate;
	}

void Create_And_Initialize_Population() {
	std::map<double, TCandidate> unique_by_fitness;
	//we need to see a variable landscape so that we can identify the extremes

//1.a insert any available hint
	for (size_t i = 0; i < mSetup.hint_count; i++) {
		TCandidate hint = Raw_To_Candidate_No_Fitness(mSetup.hints[i]);
		hint->current_fitness = hint->next_fitness = mSetup.objective(mSetup.data, hint->current.data());
		unique_by_fitness[hint->current_fitness] = std::move(hint);
	}

	//1.b insert a hyper-cube center
	{
		TCandidate center = std::make_unique<TCandidate_Data>();
		center->current = center->next = 0.5 * (mUpper_Bound + mLower_Bound);
		center->current_fitness = center->next_fitness = mSetup.objective(mSetup.data, center->current.data());
		unique_by_fitness[center->current_fitness] = std::move(center);
	}



	//2. and try to improve with the spiral, which will also generate the rest of the population
	//we do not throw away any known solution here to maximize the search-space coverage
	double best_fitness = std::numeric_limits<double>::max();
	for (size_t zoom_index = 0; zoom_index < 100; zoom_index++) {
		//find the best solution as the next spiral center
		auto global_best = std::min_element(unique_by_fitness.begin(), unique_by_fitness.end(),
			[](const auto &a, const auto &b) {
			return a.first < b.first; }
		);
		if (global_best->first >= best_fitness) break;	//improves no more

		const TUsed_Solution unit_offset = Solution_To_Unit_Offset(global_best->second->current);
		auto solutions = Generate_Spheric_Population(unit_offset);

		//move the solutions to the main population 
		for (auto &solution : solutions)
			unique_by_fitness[solution->current_fitness] = std::move(solution);
	}

	//3. eventually, keep only the desired-population-size count of the best solutions
	for (auto &solution : unique_by_fitness)
		mPopulation.push_back(std::move(solution.second));
	std::partial_sort(mPopulation.begin(), mPopulation.begin() + mSetup.population_size, mPopulation.end(), Compare_Current_Fitness);
	mPopulation.resize(mSetup.population_size);

	//4. and insert the bounds to simplify further programming		
	mPopulation.insert(mPopulation.begin(), std::move(Raw_To_Candidate_No_Fitness(mLower_Bound.data())));
	mPopulation.push_back(std::move(Raw_To_Candidate_No_Fitness(mUpper_Bound.data())));
	//note that we do not care about their fitness values as they won't be never used
}


protected:
	double Solve_Extreme_convex(std::array<double, 3> x, std::array<double, 3> y) { //returns NaN if not convex
		constexpr size_t x_min_index = 1;
		const double x_offset = x[x_min_index];
		const double y_offset = y[x_min_index];


		Eigen::Matrix<double, 2, 2> A;
		Eigen::Matrix<double, 2, 1> b;

		size_t row = 0;
		for (size_t i = 0; i < 3; i++) {
			if (i != x_min_index) {
				const double ox = x[i] - x_offset;
				A(row, 0) = ox * ox;  A(row, 1) = ox; b(row) = y[i] - y_offset;

				row++;
			}
		}

		const Eigen::Vector2d coeff = A.fullPivHouseholderQr().solve(b);
		double result = std::numeric_limits<double>::quiet_NaN();
		if (coeff[0] != 0.0) {
			result = -coeff[1] / (2.0 * coeff[0]) + x_offset;	//convex polynom
		}

		return result;
	}

	double Estimate_Extreme(const size_t index, const size_t dim) {
		TUsed_Solution reference = mPopulation[index]->current;

		std::array<double, 3> x = { mPopulation[index - 1]->current[dim],
										  mPopulation[index]->current[dim],
										  mPopulation[index + 1]->current[dim] };

		std::array<double, 3> y;

		{
			reference[dim] = x[0];
			y[0] = mSetup.objective(mSetup.data, reference.data());

			y[1] = mPopulation[index]->current_fitness;

			reference[dim] = x[2];
			y[2] = mSetup.objective(mSetup.data, reference.data());
		}

		double result = std::numeric_limits<double>::quiet_NaN();
		if ((y[1] < y[0]) && (y[1] < y[2])) {
			//by all means, this should be a convex function

			result = Solve_Extreme_convex(x, y);
				//result still may be set to NaN if the function was found to be concave or line					
		}


		if (isnan(result)) {
			//it seems that the extreme lies out of x[0] and x[2]
			//let's probe on each side and select the one with better fitness

			auto explore = [&](const size_t index) {
				x[index] += x[index] - x[1];
				x[index] = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], x[index]));
				reference[dim] = x[index];
				y[index] = mSetup.objective(mSetup.data, reference.data());
			};

			explore(0);
			explore(2);

			result = y[0] < y[2] ? x[0] : x[2];
		} else
			result = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], result));
	

		return result;			
	}


protected:
	double Estimate_Extreme_fast_but_fails_Griewank(const size_t index, const size_t dim) {
		TUsed_Solution reference = mPopulation[index]->current;

		std::array<double, 3> x = { mPopulation[index - 1]->current[dim],
										  mPopulation[index]->current[dim],
										  mPopulation[index + 1]->current[dim] };

		std::array<double, 3> y;

		auto process = [&](const size_t index) {
			//if (index != 1) x[index] = 0.5*(x[index] + x[1]);
			x[index] = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], x[index]));
			reference[dim] = x[index];
			y[index] = mSetup.objective(mSetup.data, reference.data());
		};

		constexpr double one_third = 1.0 / 3.0;
		constexpr double two_third = 2.0 / 3.0;
		x[0] += one_third * (x[1] - x[0]);
		x[1] += two_third * (x[2] - x[1]);
		process(0);
		process(1);


		return y[0] < y[1] ? x[0] : x[1];

		
		{			
			reference[dim] = x[0];
			y[0] = mSetup.objective(mSetup.data, reference.data());

			y[1] = mPopulation[index]->current_fitness;

			reference[dim] = x[2];
			y[2] = mSetup.objective(mSetup.data, reference.data());
		}

		double result = std::numeric_limits<double>::quiet_NaN();
		if ((y[1] < y[0]) && (y[1] < y[2])) {
			//by all means, this should be a convex function

//			result = Solve_Extreme_convex(x, y);
				//result still may be set to NaN if the function was found to be concave or line			

			auto explore = [&](const size_t index) {
				if (index != 1) x[index] = 0.5*(x[index] + x[1]);
				x[index] = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], x[index]));
				reference[dim] = x[index];
				y[index] = mSetup.objective(mSetup.data, reference.data());
			};

			explore(0);
			explore(2);

			result = y[0] < y[2] ? x[0] : x[2];
		}


		if (isnan(result)) {
			//it seems that the extreme lies out of x[0] and x[2]
			//let's probe on each side and select the one with better fitness

			auto explore = [&](const size_t index) {
				x[index] += x[index] - x[1];
				x[index] = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], x[index]));
				reference[dim] = x[index];
				y[index] = mSetup.objective(mSetup.data, reference.data());
			};

			explore(0);
			explore(2);

			result = y[0] < y[2] ? x[0] : x[2];
		}

		/*
		else {
			//let's try to improve anyway as not all functions behave like x^2

			auto explore = [&](const size_t index) {
				if (index != 1) x[index] = 0.5*(x[index] + x[1]);
				x[index] = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], x[index]));
				reference[dim] = x[index];
				y[index] = mSetup.objective(mSetup.data, reference.data());
			};

			explore(0);
			x[1] = result;
			explore(1);
			explore(2);
			
			const size_t result_index = std::distance(y.begin(), std::min_element(y.begin(), y.end()));
			result = x[result_index];			
		}
		*/

		return result;
	}
	

	double Estimate_Extreme_old(const size_t index, const size_t dim) {
		const std::array<double, 3> x = { mPopulation[index - 1]->current[dim],
										  mPopulation[index    ]->current[dim],
										  mPopulation[index + 1]->current[dim]};

		std::array<double, 3> y;
		{
			TUsed_Solution reference = mPopulation[index]->current;
			reference[dim] = x[0];
			y[0] = mSetup.objective(mSetup.data, reference.data());

			y[1] = mPopulation[index]->current_fitness;

			reference[dim] = x[2];
			y[2] = mSetup.objective(mSetup.data, reference.data());
		}


			
		Eigen::Matrix<double, 3, 3> A;
		Eigen::Vector3d b;


		for (size_t row = 0; row < 3; row++) {
			const double x_val = x[row];
			A(row, 0) = x_val*x_val;
			A(row, 1) = x_val;
			A(row, 2) = 1.0;
			
			b(row) = y[row];
		}

		const Eigen::Vector3d coeff = A.fullPivHouseholderQr().solve(b);
		double result = std::numeric_limits<double>::quiet_NaN();

		if (coeff[0] > 0.0) {
			//seems like a convex function - let's calculate the analytical minimum
			//result = -0.5 * coeff[1] / coeff[0]; - symbolically OK, but let's use a more numerically stable solution
			result = Solve_Extreme_convex(x, y);
				//result still may be set to NaN if the function was found to be concave or line
		}

		if (isnan(result)) {
			//Being this concanve or line, we prefere to move by two thirds closer
			//to the less fitness value.
			//We move by two thirs to avoid a premature degradation of two candidate 
			//values to a single value, if two adjacent neighbors would move towards each other
			//and would use 1/2 instead of 2/3.

			constexpr double one_third = 1.0 / 3.0;
			constexpr double two_third = 2.0 / 3.0;
			if (y[0] < y[2]) result = x[0] + one_third * (x[1] - x[0]);
			else result = x[1] + two_third * (x[2] - x[1]);
		}

		return result;
	}

protected:

	void Evolve_Population(solver::TSolver_Progress &progress) {
		std::vector<size_t> indexes(mPopulation.size() - 2);
		std::iota(indexes.begin(), indexes.end(), 1);	//note that we do not process the boundaries

		size_t improvement_count = 1;	//solve as long as we do at least one improvement										
		while ((improvement_count > 0) &&
			(progress.current_progress++ < mSetup.max_generations) &&
			(progress.cancelled == 0)) {

			improvement_count = 0; //counter provides more info than just a flag for the same costs

			//let's solve the problem per-partes by each dimension
			for (size_t dim = 0; dim < mSetup.problem_size; dim++) {
				//sort the population so that we obtain a sorted samples in one dimension of the problem
				//note that we do not sort nor process the boundaries
				std::sort(mPopulation.begin() + 1, mPopulation.end() - 1, [this, dim](const TCandidate &a, const TCandidate &b) { return a->current[dim] < b->current[dim]; });

				//attempt to improve in this particular dimension
				std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [dim, this](const size_t index) {					
					double estimate = Estimate_Extreme(index, dim);
					//estimate = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], estimate));
					mPopulation[index]->next[dim] = estimate;
					mPopulation[index]->next_fitness = mSetup.objective(mSetup.data, mPopulation[index]->next.data());
				});

				//and keep succesfull improvements
				for (auto & candidate : mPopulation) {
					if (candidate->next_fitness < candidate->current_fitness) {
						candidate->current_fitness = candidate->next_fitness;
						candidate->current = candidate->next;
						improvement_count++;
					}
				}
			}

			//if improvement_count == 0, we have either succeeded
			//or we would need another method to re-sample the search space
			//or to restart the spiral?

		}
	}

	
	

public:
	CLandscape_Pathfinder(const solver::TSolver_Setup &setup, const double angle_stepping = 3.14*2.0 / 37.0) :
		mAngle_Stepping(angle_stepping),
		mSetup(solver::Check_Default_Parameters(setup, /*100'000*/0, 100)),
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)) {
	}

	TUsed_Solution Solve(solver::TSolver_Progress &progress) {
		Create_And_Initialize_Population();

		progress.current_progress = 0;
		progress.max_progress = mSetup.max_generations;
		Evolve_Population(progress);

		auto global_best = std::min_element(mPopulation.begin()+1, mPopulation.end()-1, Compare_Current_Fitness);
		return (*global_best)->current;
	}
	   
};