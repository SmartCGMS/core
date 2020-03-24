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


#include <math.h>

#include "../../../common/rtl/AlignmentAllocator.h"


namespace fast_pathfinder_internal {
	template <typename TUsed_Solution>
	struct TCandidate {
		TUsed_Solution current, next;
		size_t leader_index = 0;	//keeping static analysis happy
		double current_fitness = std::numeric_limits<double>::quiet_NaN(), next_fitness = std::numeric_limits<double>::quiet_NaN();

		std::vector<size_t> direction_index;

		//the following members guarantee thread safety and avoid memory allocations
/*
		TUsed_Solution quadratic;
		Eigen::Matrix3d A;
		Eigen::Vector3d b, coeff;
*/
	};
}

#undef min

template <typename TUsed_Solution>
class CFast_Pathfinder : public virtual CAligned<AVX2Alignment> {
protected:
	solver::TSolver_Setup mSetup;
protected:
	double mAngle_Stepping = 0.45;
	//double mLinear_Attractor_Factor = 0.25;
	//double mLinear_Attractor_Factor = 0.5;
	//const std::array<double, 9> mDirections = { 0.9, -0.9, 0.0, 0.45, 0.45, 0.3, -0.3, 0.9 / 4.0, -0.9 / 4.0 };
	const std::array<double, 1> mDirections = { 0.5 };

	void Fill_Population_From_Candidates(const TAligned_Solution_Vector<TUsed_Solution> &candidates) {
		std::vector<double> fitness(candidates.size());
		std::vector<size_t> indexes(candidates.size());
	
		std::iota(indexes.begin(), indexes.end(), 0);
		std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [&fitness, &candidates, this](auto &index) {
				fitness[index] = mSetup.objective(mSetup.data, candidates[index].data());
		});
		
		std::partial_sort(indexes.begin(), indexes.begin() + mSetup.population_size, indexes.end(), [&fitness](const size_t a, const size_t b) {return fitness[a] < fitness[b]; });

		for (size_t i = 0; i < mSetup.population_size; i++) {
			mPopulation[i].current = candidates[indexes[i]];
			mPopulation[i].current_fitness = fitness[indexes[i]];
		}
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
	double Solve_Extreme_convex_old(std::array<double, 3> x, std::array<double, 3> y) { //assumes sorted values
		const size_t x_min_index = std::distance(x.begin(), std::min_element(x.begin(), x.end()));		

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
		else {
			//line - let's mirror it
			auto x_max = std::max_element(x.begin(), x.end());

			const double x_diff = *x_max - x_offset;

			if (coeff[1] > 0.0) {
				result = *x_max + x_diff;				
			}
			else
				result = x_offset - x_diff;			
		}
		

		return result;// +x_offset;
	}

	double Solve_Extreme_old(std::array<double, 3> x, std::array<double, 3> y) { 
		Eigen::Matrix<double, 3, 3> A;
		Eigen::Vector3d b;

		for (size_t row = 0; row < 3; row++) {
			A(row, 0) = x[row] * x[row]; 
			A(row, 1) = x[row];
			A(row, 2) = 1.0;

			b(row) = y[row];
		}


		const Eigen::Vector3d coeff = A.fullPivHouseholderQr().solve(b);
		double result = std::numeric_limits<double>::quiet_NaN();

		if (coeff[0] >= 0.0) {
			//convex or line function - let's calculate the analytical minimum
			//result = -0.5 * coeff[1] / coeff[0]; - symbolically OK, but let's use a more numerically stable solution
			result = Solve_Extreme_convex(x, y);
		}
		else {
			//concave function - let's expand beyond the least value
			//we're just guessing at this moment

			
			double min_y_value = y[0];
			double min_x_value = x[0];
			double avg_x = x[0];
			for (size_t i = 1; i < 3; i++) {
				avg_x += x[i];
				if (y[i] < min_y_value) {
					min_y_value = y[i];
					min_x_value = x[i];
				}
			}

			avg_x /= 3.0;

			const double difference = min_x_value - avg_x;
			result = min_x_value + difference;// *mLinear_Attractor_Factor;
		}

		return result;
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

	double Solve_Extreme(std::array<double, 3> x, std::array<double, 3> y, TUsed_Solution reference, size_t dim) {

		

		double result = std::numeric_limits<double>::quiet_NaN();
		if ((y[1] < y[0]) && (y[1] < y[2])) {
			//by all means, this should be a convex function 
			//its extreme should lie between x[0] and x[2] - but do not trust finite precision arithmetic!

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
		}
		else
			result = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], result));


		return result;
	}

	TUsed_Solution Calculate_Quadratic_Candidate_From_Population(const TAligned_Solution_Vector< fast_pathfinder_internal::TCandidate<TUsed_Solution>> &original_population) {

		const auto global_best = std::min_element(original_population.begin(), original_population.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution>& a, const fast_pathfinder_internal::TCandidate<TUsed_Solution>& b) {return a.next_fitness < b.next_fitness; });

		TUsed_Solution quadratic = global_best->next;
		double quadratic_fitness = global_best->next_fitness;
		

		std::vector<TUsed_Solution>  population;
		for (const auto& original : original_population)
			population.push_back(original.next);

		TUsed_Solution median_working = quadratic;

		std::vector<double> candidates;

		bool improved = true;

		while (improved) {
			improved = false;

			for (size_t dim = 0; dim < mSetup.problem_size; dim++) {
				TUsed_Solution greedy_working = quadratic;
				candidates.clear();

				std::sort(population.begin(), population.end(), [dim](const TUsed_Solution& a, const TUsed_Solution& b) { return a[dim] < b[dim]; });

				//first, calculate exact fit per-partes
				for (size_t i = 0; i < population.size() - 2; i++) {

					std::array<double, 3> x{ population[i][dim], population[i + 1][dim], population[i + 2][dim] };
					std::array<double, 3> y;

					for (size_t k = 0; k < y.size(); k++) {
						greedy_working[dim] = x[k];
						y[k] = mSetup.objective(mSetup.data, greedy_working.data());
					}


					const double found_extreme = std::min(mUpper_Bound[dim], std::max(mLower_Bound[dim], Solve_Extreme(x, y, greedy_working, dim)));

					candidates.push_back(found_extreme);
					greedy_working[dim] = found_extreme;

					const double working_fitness = mSetup.objective(mSetup.data, greedy_working.data());
					if (working_fitness < quadratic_fitness) {
						quadratic = greedy_working;
						quadratic_fitness = working_fitness;
						improved = true;
					}


				}

				std::sort(candidates.begin(), candidates.end());
				median_working[dim] = candidates[candidates.size() / 2];
			}

			const double median_working_fitness = mSetup.objective(mSetup.data, median_working.data());
			if (median_working_fitness < quadratic_fitness) {
				quadratic = median_working;
				quadratic_fitness = median_working_fitness;
				improved = true;
			}

		}

		return quadratic;
	}




protected:
	TAligned_Solution_Vector<fast_pathfinder_internal::TCandidate<TUsed_Solution>> mPopulation;	
protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;

	TUsed_Solution Solution_To_Unit_Offset(const TUsed_Solution &solution) {
		const auto bounds_range = mUpper_Bound - mLower_Bound;
		TUsed_Solution unit_offset = solution - mLower_Bound;
		for (size_t j = 0; j < mSetup.problem_size; j++) {
			if (bounds_range[j] != 0.0) unit_offset[j] /= bounds_range[j];
				else unit_offset[j] = 0.0;
		}

		return unit_offset;
	}

	
	bool Evolve_Solution(fast_pathfinder_internal::TCandidate<TUsed_Solution>& candidate_solution, const fast_pathfinder_internal::TCandidate<TUsed_Solution>& leading_solution) {
		//returns true, if found_solution has better found fitness than the current fitness of the candidate_solution

		bool improved = false;

		TAligned_Solution_Vector< fast_pathfinder_internal::TCandidate<TUsed_Solution>> temporal_population;
		temporal_population.push_back(candidate_solution);
		temporal_population.push_back(leading_solution);

		TUsed_Solution difference = candidate_solution.current - leading_solution.current;

		auto push_sample = [&](const double offset) {
			fast_pathfinder_internal::TCandidate<TUsed_Solution> sample;
			sample.current = candidate_solution.current + offset * difference;
			sample.next = sample.current = mUpper_Bound.min(mLower_Bound.max(sample.current));
			sample.current_fitness = sample.next_fitness = mSetup.objective(mSetup.data, sample.current.data());			
			temporal_population.push_back(sample);

			if (sample.current_fitness < candidate_solution.next_fitness) {
				candidate_solution.next = sample.current;
				candidate_solution.next_fitness = sample.current_fitness;
				improved = true;
			}
		};


		push_sample(-0.25);
		//push_sample(-0.125);
		//push_sample(0.125);
		push_sample(0.25);
		//push_sample(0.375);
		push_sample(0.5);
		//push_sample(0.625);
		push_sample(0.75);
		//push_sample(0.875);
		//push_sample(1.15);
		push_sample(1.25);

		/*
		fast_pathfinder_internal::TCandidate<TUsed_Solution> quadratic_candidate;
		{
			const TUsed_Solution difference = candidate_solution.current - leading_solution.current;
			quadratic_candidate.next = candidate_solution.current;
			for (size_t i = 0; i < mSetup.problem_size; i++)
				quadratic_candidate.next[i] += difference[i] * mDirections[candidate_solution.direction_index[i]];			 
			quadratic_candidate.next = mUpper_Bound.min(mLower_Bound.max(quadratic_candidate.next));
		}

		quadratic_candidate.current = quadratic_candidate.next;
		quadratic_candidate.current_fitness = quadratic_candidate.next_fitness = mSetup.objective(mSetup.data, quadratic_candidate.next.data());

		temporal_population.push_back(quadratic_candidate);

		*/


		TUsed_Solution eval = Calculate_Quadratic_Candidate_From_Population(temporal_population);
		const double eval_fitness = mSetup.objective(mSetup.data, eval.data());

		
		if (eval_fitness < candidate_solution.next_fitness) {
			candidate_solution.next_fitness = eval_fitness;
			candidate_solution.next = eval;
			improved = true;
		}

		return improved;
	}


	TUsed_Solution Evolve_Population(solver::TSolver_Progress &progress) {

		while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == FALSE)) {

			//update the progress
			auto global_best = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });
			progress.best_metric = global_best->current_fitness;

			//2. Calculate the next vectors and their fitness 
			//In this step, current is read-only and next is write-only => no locking is needed
			//as each next will be written just once.
			//We assume that parallelization cost will get amortized			
			std::for_each(std::execution::par_unseq, mPopulation.begin(), mPopulation.end(), [=](auto &candidate_solution) {
					
				//for (size_t i = 0; i < mPopulation.size(); i++) 
				//	Evolve_Solution(candidate_solution, mPopulation[i]);
				

				if (!Evolve_Solution(candidate_solution, mPopulation[candidate_solution.leader_index])) {

					candidate_solution.leader_index++;
					if (candidate_solution.leader_index >= mPopulation.size()) candidate_solution.leader_index = 0;

					if (!Evolve_Solution(candidate_solution, *global_best))
						for (size_t i = 0; i < mSetup.problem_size; i++) {
							candidate_solution.direction_index[i]++;
							if (candidate_solution.direction_index[i] >= mDirections.size()) candidate_solution.direction_index[i] = 0;
						}
				}

				
				
			});

			global_best = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.next_fitness < b.next_fitness; });
			auto global_worst = std::max_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.next_fitness < b.next_fitness; });

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

			TUsed_Solution quadratic = Calculate_Quadratic_Candidate_From_Population(mPopulation);

			const double q_f = mSetup.objective(mSetup.data, quadratic.data());

			if (q_f < global_worst->next_fitness) {
				global_worst->next = quadratic;
				global_worst->next_fitness = q_f;
				for (auto &dir : global_worst->direction_index)
					dir = 0;
				global_worst->leader_index = static_cast<size_t>(std::distance(mPopulation.begin(), global_best));
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

	void Create_And_Initialize_Population() {
		mPopulation.resize(mSetup.population_size);

		//1. create the initial population

		//a) by creating deterministically generated solutions
		{
			TUsed_Solution unit_offset;
			unit_offset.resize(Eigen::NoChange, mSetup.problem_size);
			unit_offset.setConstant(0.5);

			TAligned_Solution_Vector<TUsed_Solution> solutions;
			TAligned_Solution_Vector<TUsed_Solution> best_solutions;
			solutions = Generate_Spheric_Population(unit_offset);

			//b) and appending the supplied hints
			for (size_t i = 0; i < mSetup.hint_count; i++) {
				solutions.push_back(mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], mSetup.problem_size))));//also ensure the bounds
			}

			Fill_Population_From_Candidates(solutions);

			double best_fitness = std::numeric_limits<double>::max();

			for (size_t zoom_index = 0; zoom_index < 100; zoom_index++) {

				//calculate the metrics
				for (auto &solution : mPopulation) {
					solution.next_fitness = solution.current_fitness;// = mSetup.objective(mSetup.data, solution.current.data());
					solution.next = solution.current;
				}
				
				//find the best as the next offset - needs recalculation
				auto global_best = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const fast_pathfinder_internal::TCandidate<TUsed_Solution> &a, const fast_pathfinder_internal::TCandidate<TUsed_Solution> &b) {return a.current_fitness < b.current_fitness; });

				//try to improve by quadratic
				TUsed_Solution quadratic = Calculate_Quadratic_Candidate_From_Population(mPopulation);
//				for (size_t j = 0; j < mSetup.problem_size; j++)
//					if (isnan(quadratic[j])) quadratic[j] = global_best->current[j];

				const double q_f = mSetup.objective(mSetup.data, quadratic.data());
				if (q_f < global_best->current_fitness) {
					global_best->current_fitness = q_f;
					global_best->current = quadratic;
				}

				best_solutions.push_back(global_best->current);

				if (global_best->current_fitness < best_fitness) best_fitness = global_best->current_fitness;
				else break;

				unit_offset = Solution_To_Unit_Offset(global_best->current);
				
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

//			solution.quadratic.resize(Eigen::NoChange, mSetup.problem_size);
//			solution.A.setConstant(1.0);

			solution.direction_index.assign(mSetup.problem_size, 0);
		}
		mPopulation[0].leader_index = 1;	//so that it does not point to itself

	}


public:
	CFast_Pathfinder(const solver::TSolver_Setup &setup, const double angle_stepping = 3.14*2.0 / 37.0) :
		mSetup(solver::Check_Default_Parameters(setup, /*100'000*/0, 100)),
		mAngle_Stepping(angle_stepping),
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)) {
	}

	TUsed_Solution Solve(solver::TSolver_Progress &progress) {
		Create_And_Initialize_Population();


		progress.current_progress = 0;
		progress.max_progress = mSetup.max_generations;

		TUsed_Solution best_solution = Evolve_Population(progress);
		double best_fitness = mSetup.objective(mSetup.data, best_solution.data());

		while ((progress.current_progress < mSetup.max_generations) && (progress.cancelled == FALSE)) {

			const TUsed_Solution unit_offset = Solution_To_Unit_Offset(best_solution);
			const TAligned_Solution_Vector<TUsed_Solution> solutions = Generate_Spheric_Population(unit_offset);
			Fill_Population_From_Candidates(solutions);

			for (auto &solution : mPopulation) {

				solution.leader_index = 0;	//as the population is sorted, zero is the best leader

				//the main solving algorithm expects that next contains valid values from the last run of the generation
				solution.next = solution.current;
				solution.next_fitness = solution.current_fitness;

				std::fill(solution.direction_index.begin(), solution.direction_index.end(), 0);
			}
			mPopulation[0].leader_index = 1;	//so that it does not point to itself


			const TUsed_Solution local_solution = Evolve_Population(progress);
			const double local_fitness = mSetup.objective(mSetup.data, local_solution.data());

			if (local_fitness < best_fitness) {
				best_fitness = local_fitness;
				best_solution = local_solution;
			}
			else break;
		}

		return best_solution;

	}



};