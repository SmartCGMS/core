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
#include <random>
#include <algorithm>
#include <numeric>
#include <map>
#include <ctime>
#include <chrono>
#include <set>

#undef max
#undef min

#include <scgms/rtl/SolverLib.h>
#include <scgms/utils/DebugHelper.h>

namespace mutation
{
	template <typename TUsed_Solution>
	struct TCandidate_Solution {
		// current parameter vector
		TUsed_Solution current;
		// current parameter vector fitness
		solver::TFitness fitness = solver::Nan_Fitness;
	};

	// individuals vector used in calculations
	template<typename TUsed_Solution>
	using TIndividuals_Vector = TAligned_Solution_Vector<mutation::TCandidate_Solution<TUsed_Solution>>;
}

template <typename TUsed_Solution, typename TRandom_Device = std::random_device>
class COne_To_N_Mutation
{
	protected:
		// mutation chance
		static constexpr double p_mut = 0.7;

	protected:
		const solver::TSolver_Setup mSetup;
		const TUsed_Solution mLower_Bound;
		const TUsed_Solution mUpper_Bound;
		const TUsed_Solution mBound_Diff;
		mutation::TIndividuals_Vector<TUsed_Solution> mPopulation;

	protected:
		inline static TRandom_Device mRandom_Generator;
		inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };

	public:
		COne_To_N_Mutation(const solver::TSolver_Setup &setup) :
			mSetup(solver::Check_Default_Parameters(setup, 1'000, 100)),
			mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)),
			mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
			mBound_Diff(mUpper_Bound - mLower_Bound)
		{
			Init_Population();
		}

		void Init_Population() {
			mPopulation.resize(std::max(mSetup.population_size, static_cast<decltype(mSetup.population_size)>(5)));

			// create the initial swarm using supplied hints; fill up to a half of a swarm with hints
			const size_t initialized_count = std::min(mPopulation.size() / 2, mSetup.hint_count);

			for (size_t i = 0; i < initialized_count; i++)
				mPopulation[i].current = Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], mSetup.problem_size);

			for (size_t i = initialized_count; i < mPopulation.size(); i++)
			{
				TUsed_Solution tmp;

				// this helps when we use generic solution vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
				// optimized away in compile time
				tmp.resize(Eigen::NoChange, mSetup.problem_size);

				for (size_t j = 0; j < mSetup.problem_size; j++)
					tmp[j] = mUniform_Distribution_dbl(mRandom_Generator);

				mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(mBound_Diff);
			}

			// calculate initial fitness
			std::for_each(std::execution::par_unseq, mPopulation.begin(), mPopulation.end(), [this](auto& candidate_solution) {
				if (mSetup.objective(mSetup.data, 1, candidate_solution.current.data(), candidate_solution.fitness.data()) != TRUE)
				{
					for (auto& elem : candidate_solution.fitness)
						elem = std::numeric_limits<double>::quiet_NaN();	// sanitize on error
				}
			});

			std::partial_sort(mPopulation.begin(), mPopulation.begin() + 1, mPopulation.end(), [](const auto& a, const auto& b) {
				return a.fitness < b.fitness;
			});
		}

		TUsed_Solution Solve(solver::TSolver_Progress &progress) {

			progress.current_progress = 0;
			progress.max_progress = mSetup.max_generations;
	
			while (progress.current_progress++ < mSetup.max_generations && (progress.cancelled == FALSE))
			{
				std::for_each(std::execution::par_unseq, mPopulation.begin() + 1, mPopulation.end(), [this](auto& candidate_solution) {
					candidate_solution.current = mPopulation[0].current;

					for (size_t j = 0; j < mSetup.problem_size; j++)
					{
						if (mUniform_Distribution_dbl(mRandom_Generator) < p_mut)
							candidate_solution.current[j] = mLower_Bound[j] + mBound_Diff[j] * mUniform_Distribution_dbl(mRandom_Generator);
					}
				});

				std::for_each(std::execution::par_unseq, mPopulation.begin(), mPopulation.end(), [this](auto& candidate_solution) {
					if (mSetup.objective(mSetup.data, 1, candidate_solution.current.data(), candidate_solution.fitness.data()) != TRUE)
					{
						for (auto& elem : candidate_solution.fitness)
							elem = std::numeric_limits<double>::quiet_NaN();	// sanitize on error
					}
				});

				std::partial_sort(mPopulation.begin(), mPopulation.begin() + 1, mPopulation.end(), [](const auto& a, const auto& b) {
					return a.fitness < b.fitness;
				});

				progress.current_progress++;
				progress.best_metric = mPopulation[0].fitness;
			}

			return mPopulation[0].current;
		}
};
