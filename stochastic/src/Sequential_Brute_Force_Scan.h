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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <climits>

#include "solution.h"
#include "../../../common/rtl/SolverLib.h"

template <typename TUsed_Solution>
class CSequential_Brute_Force_Scan {
protected:
	const NFitness_Strategy mFitness_Strategy = NFitness_Strategy::Master;
protected:
	template <typename TSolution>
	struct TCandidate_Solution {
		TSolution solution;
		solver::TFitness fitness = solver::Nan_Fitness;
	};
protected:
	std::vector<double> mBulk_Solutions;
	std::vector<double> mBulk_Fitness;

	void Prepare_Next_Solutions(const size_t param_idx, const TUsed_Solution& solution) {
		const double* src = solution.data();
		double* dst = mBulk_Solutions.data();
		for (size_t i = 0; i < mStepping.size(); i++) {
			std::copy(src, src + mSetup.problem_size, dst);
			dst[param_idx] = mStepping[i][param_idx];
			dst += mSetup.problem_size;
		}
	}


	size_t Next_Best_Fitness() {
		size_t best_idx = 0;
		solver::TFitness* best_fitness = reinterpret_cast<solver::TFitness*>(mBulk_Fitness.data());
		solver::TFitness* fitness_to_compare = best_fitness + 1;
		for (size_t i = 1; i < mStepping.size(); i++) {			

			if (Compare_Solutions(*fitness_to_compare, *best_fitness, mSetup.problem_size, NFitness_Strategy::Master)) {
				best_idx = i;
				best_fitness = fitness_to_compare;
			}

			fitness_to_compare++;
		}

		return best_idx;
	}

protected:
	const TUsed_Solution mLower_Bound;
	const TUsed_Solution mUpper_Bound;
	std::vector<TUsed_Solution> mStepping;
protected:
	solver::TSolver_Setup mSetup;
public:
	CSequential_Brute_Force_Scan(const solver::TSolver_Setup &setup) :
		mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)), mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
		mSetup(solver::Check_Default_Parameters(setup, /*100'000*/0, 100)) {

		//a) by storing suggested params
		for (size_t i = 0; i < mSetup.hint_count; i++) {	
			mStepping.push_back(mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size))));//also ensure the bounds
		}

		TUsed_Solution stepping;
		stepping.resize(mUpper_Bound.cols());
		for (auto i = 0; i < mUpper_Bound.cols(); i++) {
			stepping(i) = (mUpper_Bound(i) - mLower_Bound(i)) / static_cast<double>(mSetup.population_size);
		}

		TUsed_Solution step = mLower_Bound;
		mStepping.push_back(step);
		for (size_t i = 0; i < mSetup.population_size; i++) {
			step += stepping;
			mStepping.push_back(step);
		}

		mBulk_Solutions.resize(mSetup.problem_size * mStepping.size());
		mBulk_Fitness.resize(solver::Maximum_Objectives_Count * mStepping.size());
		std::fill(mBulk_Fitness.begin(), mBulk_Fitness.end(), std::numeric_limits<double>::quiet_NaN());
	}

	

	TUsed_Solution Solve(solver::TSolver_Progress &progress) {

		progress = solver::Null_Solver_Progress;

		progress.current_progress = 0;
		progress.max_progress = mStepping.size();


		TCandidate_Solution<TUsed_Solution> best_solution;
		best_solution.solution = mStepping[0];
		best_solution.fitness = solver::Nan_Fitness;

		if (mSetup.objective(mSetup.data, 1, best_solution.solution.data(), best_solution.fitness.data()) != TRUE)
			return best_solution.solution;			//TODO: this is not necessarily the best solution, just the first one!
		
		progress.best_metric = best_solution.fitness;

				bool improved = true;

		size_t improved_counter = 0;
		while (improved && (improved_counter++ < mSetup.max_generations)) {
			improved = false;
			progress.current_progress = 0;

			for (size_t param_idx = 0; param_idx < mSetup.problem_size; param_idx++) {
				if (progress.cancelled != FALSE) break;

				if (mLower_Bound[param_idx] != mUpper_Bound[param_idx]) {
					Prepare_Next_Solutions(param_idx, best_solution.solution);
					if (mSetup.objective(mSetup.data, mStepping.size(), mBulk_Solutions.data(), mBulk_Fitness.data()) == TRUE) {
						const size_t local_best_idx = Next_Best_Fitness();
						const solver::TFitness& local_best_fitness = *(reinterpret_cast<solver::TFitness*>(mBulk_Fitness.data()) + local_best_idx);
						if (Compare_Solutions(local_best_fitness, best_solution.fitness, mSetup.problem_size, NFitness_Strategy::Master)) {
							best_solution.fitness = local_best_fitness;
							
							double* local_solution = mBulk_Solutions.data() + local_best_idx * mSetup.problem_size;
							std::copy(local_solution, local_solution + mSetup.problem_size, best_solution.solution.data());


							progress.best_metric = local_best_fitness;

							improved = true;
						}
					}


					progress.current_progress++;										
				}
			}
		}

		return best_solution.solution;
	}
};