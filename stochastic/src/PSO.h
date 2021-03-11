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

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/utils/DebugHelper.h"

namespace pso
{
	template <typename TUsed_Solution>
	struct TCandidate_Solution {
		// current parameter vector
		TUsed_Solution current;
		// local memory - best known position
		TUsed_Solution best;
		// current particle velocity vector
		TUsed_Solution velocity;
		// current parameter vector fitness
		double current_fitness = std::numeric_limits<double>::max();
		// local memory - best known position fitness
		double best_fitness = std::numeric_limits<double>::max();
	};

	// swarm vector used in calculations
	template<typename TUsed_Solution>
	using TSwarm_Vector = TAligned_Solution_Vector<pso::TCandidate_Solution<TUsed_Solution>>;

	// base for CRTP type of swarm generator
	template<typename TUsed_Solution, typename TRandom_Device, typename TPop_Gen>
	class ISwarm_Generator
	{
		protected:
			inline static TRandom_Device mRandom_Generator;
			inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };

		public:
			void Generate_Swarm(TSwarm_Vector<TUsed_Solution>& target, size_t begin, size_t end, const size_t problem_size, const TUsed_Solution& lower_bound, const TUsed_Solution& upper_bound) {
				static_cast<TPop_Gen*>(this)->Generate_Swarm_Impl(target, begin, end, problem_size, lower_bound, upper_bound);
			}
	};

	// Random swarm generator - generates random swarm members within given bounds
	template<typename TUsed_Solution, typename TRandom_Device>
	class CRandom_Swarm_Generator final : public ISwarm_Generator<TUsed_Solution, TRandom_Device, CRandom_Swarm_Generator<TUsed_Solution, TRandom_Device>>
	{
		protected:
			inline void Generate_Random_Candidate(TSwarm_Vector<TUsed_Solution>& target, size_t idx, const size_t problem_size, const TUsed_Solution& lower_bound, const TUsed_Solution& upper_bound) {
				TUsed_Solution tmp;

				// this helps when we use generic solution vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
				// optimized away in compile time
				tmp.resize(Eigen::NoChange, problem_size);

				for (size_t j = 0; j < problem_size; j++)
					tmp[j] = mUniform_Distribution_dbl(mRandom_Generator);

				target[idx].current = lower_bound + tmp.cwiseProduct(upper_bound - lower_bound);
			}

		public:
			void Generate_Swarm_Impl(TSwarm_Vector<TUsed_Solution>& target, size_t begin, size_t end, const size_t problem_size, const TUsed_Solution& lower_bound, const TUsed_Solution& upper_bound) {
				for (size_t i = begin; i < end; i++)
					Generate_Random_Candidate(target, i, problem_size, lower_bound, upper_bound);
			}
	};

	// Diagonal swarm generator - generates swarm members as equally-distributed points on first diagonal
	template<typename TUsed_Solution, typename TRandom_Device>
	class CDiagonal_Swarm_Generator final : public ISwarm_Generator<TUsed_Solution, TRandom_Device, CDiagonal_Swarm_Generator<TUsed_Solution, TRandom_Device>>
	{
		public:
			void Generate_Swarm_Impl(TSwarm_Vector<TUsed_Solution>& target, size_t begin, size_t end, const size_t problem_size, const TUsed_Solution& lower_bound, const TUsed_Solution& upper_bound) {
				const double step = 1.0 / (end - begin);
				for (size_t i = begin; i < end; i++)
					target[i].current = lower_bound + static_cast<double>(i - begin) * step * (upper_bound - lower_bound);
			}
	};

	// Cross-diagonal swarm generator - generates swarm members as equally-distributed points on two diagonals
	template<typename TUsed_Solution, typename TRandom_Device>
	class CCross_Diagonal_Swarm_Generator final : public ISwarm_Generator<TUsed_Solution, TRandom_Device, CCross_Diagonal_Swarm_Generator<TUsed_Solution, TRandom_Device>>
	{
		public:
			void Generate_Swarm_Impl(TSwarm_Vector<TUsed_Solution>& target, size_t begin, size_t end, const size_t problem_size, const TUsed_Solution& lower_bound, const TUsed_Solution& upper_bound) {
				const size_t half = begin + (end - begin) / 2;
				const double step = 2.0 / (end - begin);

				const auto bounds_range = upper_bound - lower_bound;

				for (size_t i = begin; i < half; i++)
					target[i].current = lower_bound + static_cast<double>(i - begin) * step * bounds_range;

				for (size_t i = half; i < end; i++) {
					target[i].current = lower_bound + static_cast<double>(i - half) * step * bounds_range;

					for (size_t j = problem_size / 2; j < problem_size; j++) {
						target[i].current[j] = lower_bound[j] + (upper_bound[j] - target[i].current[j]);
					}
				}
			}
	};



	// base for CRTP type of velocity modifier
	template<typename TUsed_Solution, typename TRandom_Device, typename TMod>
	class IVelocity_Modifier
	{
		protected:
			inline static TRandom_Device mRandom_Generator;
			inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };

		public:
			auto Generate_Velocity_Change(const size_t problem_size) {
				return static_cast<TMod*>(this)->Generate_Velocity_Change_Impl(problem_size);
			}
	};

	// single coefficient velocity modifier - one single random value is used for each dimension of local and global memory
	template<typename TUsed_Solution, typename TRandom_Device>
	class CSingle_Coefficient_Velocity_Modifier final : public IVelocity_Modifier<TUsed_Solution, TRandom_Device, CSingle_Coefficient_Velocity_Modifier<TUsed_Solution, TRandom_Device>>
	{
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				double val = mUniform_Distribution_dbl(mRandom_Generator);

				return std::pair<double, double>(val, val);
			}
	};

	// dual coefficient velocity modifier - two random values are used for each dimension, one for local and one for global memory
	template<typename TUsed_Solution, typename TRandom_Device>
	class CDual_Coefficient_Velocity_Modifier final : public IVelocity_Modifier<TUsed_Solution, TRandom_Device, CDual_Coefficient_Velocity_Modifier<TUsed_Solution, TRandom_Device>>
	{
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				double val1 = mUniform_Distribution_dbl(mRandom_Generator);
				double val2 = mUniform_Distribution_dbl(mRandom_Generator);

				return std::pair<double, double>(val1, val2);
			}
	};

	// single coefficient vector velocity modifier - one vector of random values is used for local and global memory
	template<typename TUsed_Solution, typename TRandom_Device>
	class CSingle_Coefficient_Vector_Velocity_Modifier final : public IVelocity_Modifier<TUsed_Solution, TRandom_Device, CSingle_Coefficient_Vector_Velocity_Modifier<TUsed_Solution, TRandom_Device>>
	{
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				TUsed_Solution rndvec, rndvec2;
				rndvec.resize(Eigen::NoChange, problem_size);

				for (size_t i = 0; i < problem_size; i++) {
					rndvec[i] = mUniform_Distribution_dbl(mRandom_Generator);
				}

				rndvec2 = rndvec;

				return std::make_pair<TUsed_Solution, TUsed_Solution>(std::move(rndvec), std::move(rndvec2));
			}
	};

	// single coefficient vector velocity modifier - two vectors of random values are used, one for local and one for global memory
	template<typename TUsed_Solution, typename TRandom_Device>
	class CDual_Coefficient_Vector_Velocity_Modifier final : public IVelocity_Modifier<TUsed_Solution, TRandom_Device, CDual_Coefficient_Vector_Velocity_Modifier<TUsed_Solution, TRandom_Device>>
	{
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				TUsed_Solution rndvec1, rndvec2;
				rndvec1.resize(Eigen::NoChange, problem_size);
				rndvec2.resize(Eigen::NoChange, problem_size);

				for (size_t i = 0; i < problem_size; i++) {
					rndvec1[i] = mUniform_Distribution_dbl(mRandom_Generator);
					rndvec2[i] = mUniform_Distribution_dbl(mRandom_Generator);
				}

				return std::make_pair<TUsed_Solution, TUsed_Solution>(std::move(rndvec1), std::move(rndvec2));
			}
	};
}


template <typename TUsed_Solution, typename TRandom_Device = std::random_device,
	template<typename, typename> typename TSwarm_Generator = pso::CRandom_Swarm_Generator,
	template<typename, typename> typename TVelocity_Modifier = pso::CSingle_Coefficient_Velocity_Modifier>
class CPSO
{
	protected:
		// inertia factor (how much velocity the particle retains from previous generation)
		static constexpr double omega = 0.7298;
		// local memory attraction (how much the particle wants to return to its best known position)
		static constexpr double phi_p = 2.05;
		// global memory attraction (how much the particle wants to go to swarm's best known position)
		static constexpr double phi_g = 1.55;
		// maximum velocity factor (the maximum length of velocity vector in fractions of search space)
		static constexpr double max_velocity = 0.4;

	protected:
		const solver::TSolver_Setup mSetup;
		const TUsed_Solution mLower_Bound;
		const TUsed_Solution mUpper_Bound;
		const TUsed_Solution mVelocity_Lower_Bound, mVelocity_Upper_Bound;
		pso::TSwarm_Vector<TUsed_Solution> mSwarm;

		typename pso::TSwarm_Vector<TUsed_Solution>::iterator mBest_Itr;
		double mBest_Fitness = std::numeric_limits<double>::max();

		// updates global memory with best known position of the whole swarm
		inline void Update_Best_Candidate() {
			mBest_Itr = std::min_element(std::execution::par_unseq, mSwarm.begin(), mSwarm.end(), [](const pso::TCandidate_Solution<TUsed_Solution>& a, const pso::TCandidate_Solution<TUsed_Solution>& b) {
				return a.best_fitness < b.best_fitness;
			});

			mBest_Fitness = mBest_Itr->best_fitness;
		}

	protected:
		inline static TRandom_Device mRandom_Generator;
		inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };

	public:
		CPSO(const solver::TSolver_Setup &setup) : 
			mSetup(solver::Check_Default_Parameters(setup, 1'000, 100)),
			mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)),
			mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
			mVelocity_Lower_Bound(- max_velocity * (mUpper_Bound - mLower_Bound)),
			mVelocity_Upper_Bound(  max_velocity * (mUpper_Bound - mLower_Bound))
		{
			mSwarm.resize(std::max(mSetup.population_size, 5ULL));

			// create the initial swarm using supplied hints; fill up to a half of a swarm with hints
			const size_t initialized_count = std::min(mSwarm.size() / 2, mSetup.hint_count);

			for (size_t i = 0; i < initialized_count; i++)
				mSwarm[i].current = Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], setup.problem_size);

			// complement the rest with generated candidates
			TSwarm_Generator<TUsed_Solution, TRandom_Device> gen;
			gen.Generate_Swarm(mSwarm, initialized_count, mSwarm.size(), mSetup.problem_size, mLower_Bound, mUpper_Bound);

			// calculate initial fitness
			std::for_each(std::execution::par_unseq, mSwarm.begin(), mSwarm.end(), [this](auto &candidate_solution) {
				candidate_solution.current_fitness = mSetup.objective(mSetup.data, candidate_solution.current.data());

				// current is also the best known
				candidate_solution.best_fitness = candidate_solution.current_fitness;
				candidate_solution.best = candidate_solution.current;

				candidate_solution.velocity.resize(Eigen::NoChange, mSetup.problem_size);

				// initial velocity is random within bounds on both sides

				TUsed_Solution rand_init;
				rand_init.resize(Eigen::NoChange, mSetup.problem_size);

				for (size_t j = 0; j < mSetup.problem_size; j++)
					rand_init[j] = mUniform_Distribution_dbl(mRandom_Generator);

				candidate_solution.velocity = mVelocity_Lower_Bound + rand_init * (mVelocity_Upper_Bound - mVelocity_Lower_Bound);
			});

			// select swarm best candidate and store it
			Update_Best_Candidate();
		}

		TUsed_Solution Solve(solver::TSolver_Progress &progress) {

			progress.current_progress = 0;
			progress.max_progress = mSetup.max_generations;
	
			const size_t solution_size = mSwarm[0].current.cols();
			TVelocity_Modifier<TUsed_Solution, TRandom_Device> mod;

			while ((progress.current_progress++ < mSetup.max_generations) && (progress.cancelled == FALSE))
			{
				// update the progress
				progress.best_metric = mBest_Fitness;

				std::for_each(std::execution::par_unseq, mSwarm.begin(), mSwarm.end(), [=, &mod](auto &candidate_solution) {

					// generate velocity update vectors/scalars (depends on chosen velocity update operator)
					auto [r_p, r_g] = mod.Generate_Velocity_Change(solution_size);

					// update velocity and ensure bounds
					candidate_solution.velocity = mVelocity_Upper_Bound.min(mVelocity_Lower_Bound.max(
						omega * candidate_solution.velocity
						+ phi_p * r_p * (candidate_solution.best - candidate_solution.current)
						+ phi_g * r_g * (mBest_Itr->best - candidate_solution.current)
					));

					// move particle by its velocity vector and ensure bounds
					candidate_solution.current = mUpper_Bound.min(mLower_Bound.max(candidate_solution.current + candidate_solution.velocity));

					// evaluate candidate solution
					candidate_solution.current_fitness = mSetup.objective(mSetup.data, candidate_solution.current.data());

					// update best solution
					if (candidate_solution.current_fitness < candidate_solution.best_fitness)
					{
						candidate_solution.best = candidate_solution.current;
						candidate_solution.best_fitness = candidate_solution.current_fitness;
					}
				});

				// update swarm-best candidate
				Update_Best_Candidate();
			}

			return mBest_Itr->best;
		}
};
