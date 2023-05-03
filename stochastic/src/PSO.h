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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
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
		solver::TFitness current_fitness = solver::Nan_Fitness;
		// local memory - best known position fitness
		solver::TFitness best_fitness = solver::Nan_Fitness;
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
		using TBase = ISwarm_Generator<TUsed_Solution, TRandom_Device, CRandom_Swarm_Generator<TUsed_Solution, TRandom_Device>>;
		protected:
			inline void Generate_Random_Candidate(TSwarm_Vector<TUsed_Solution>& target, size_t idx, const size_t problem_size, const TUsed_Solution& lower_bound, const TUsed_Solution& upper_bound) {
				TUsed_Solution tmp;

				// this helps when we use generic solution vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
				// optimized away in compile time
				tmp.resize(Eigen::NoChange, problem_size);

				for (size_t j = 0; j < problem_size; j++)
					tmp[j] = TBase::mUniform_Distribution_dbl(TBase::mRandom_Generator);

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
		using TBase = IVelocity_Modifier<TUsed_Solution, TRandom_Device, CSingle_Coefficient_Velocity_Modifier<TUsed_Solution, TRandom_Device>>;
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				double val = TBase::mUniform_Distribution_dbl(TBase::mRandom_Generator);

				return std::pair<double, double>(val, val);
			}
	};

	// dual coefficient velocity modifier - two random values are used for each dimension, one for local and one for global memory
	template<typename TUsed_Solution, typename TRandom_Device>
	class CDual_Coefficient_Velocity_Modifier final : public IVelocity_Modifier<TUsed_Solution, TRandom_Device, CDual_Coefficient_Velocity_Modifier<TUsed_Solution, TRandom_Device>>
	{
		using TBase = IVelocity_Modifier<TUsed_Solution, TRandom_Device, CDual_Coefficient_Velocity_Modifier<TUsed_Solution, TRandom_Device>>;
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				double val1 = TBase::mUniform_Distribution_dbl(TBase::mRandom_Generator);
				double val2 = TBase::mUniform_Distribution_dbl(TBase::mRandom_Generator);

				return std::pair<double, double>(val1, val2);
			}
	};

	// single coefficient vector velocity modifier - one vector of random values is used for local and global memory
	template<typename TUsed_Solution, typename TRandom_Device>
	class CSingle_Coefficient_Vector_Velocity_Modifier final : public IVelocity_Modifier<TUsed_Solution, TRandom_Device, CSingle_Coefficient_Vector_Velocity_Modifier<TUsed_Solution, TRandom_Device>>
	{
		using TBase = IVelocity_Modifier<TUsed_Solution, TRandom_Device, CSingle_Coefficient_Vector_Velocity_Modifier<TUsed_Solution, TRandom_Device>>;
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				TUsed_Solution rndvec, rndvec2;
				rndvec.resize(Eigen::NoChange, problem_size);

				for (size_t i = 0; i < problem_size; i++) {
					rndvec[i] = TBase::mUniform_Distribution_dbl(TBase::mRandom_Generator);
				}

				rndvec2 = rndvec;

				return std::make_pair<TUsed_Solution, TUsed_Solution>(std::move(rndvec), std::move(rndvec2));
			}
	};

	// single coefficient vector velocity modifier - two vectors of random values are used, one for local and one for global memory
	template<typename TUsed_Solution, typename TRandom_Device>
	class CDual_Coefficient_Vector_Velocity_Modifier final : public IVelocity_Modifier<TUsed_Solution, TRandom_Device, CDual_Coefficient_Vector_Velocity_Modifier<TUsed_Solution, TRandom_Device>>
	{
		using TBase = IVelocity_Modifier<TUsed_Solution, TRandom_Device, CDual_Coefficient_Vector_Velocity_Modifier<TUsed_Solution, TRandom_Device>>;
		public:
			auto Generate_Velocity_Change_Impl(const size_t problem_size)
			{
				TUsed_Solution rndvec1, rndvec2;
				rndvec1.resize(Eigen::NoChange, problem_size);
				rndvec2.resize(Eigen::NoChange, problem_size);

				for (size_t i = 0; i < problem_size; i++) {
					rndvec1[i] = TBase::mUniform_Distribution_dbl(TBase::mRandom_Generator);
					rndvec2[i] = TBase::mUniform_Distribution_dbl(TBase::mRandom_Generator);
				}

				return std::make_pair<TUsed_Solution, TUsed_Solution>(std::move(rndvec1), std::move(rndvec2));
			}
	};
}


template <typename TUsed_Solution, typename TRandom_Device = std::random_device,
	template<typename, typename> typename TSwarm_Generator = pso::CRandom_Swarm_Generator,
	template<typename, typename> typename TVelocity_Modifier = pso::CSingle_Coefficient_Velocity_Modifier,
	bool repulsory = false>
class CPSO
{
	protected:
		// inertia factor (how much velocity the particle retains from previous generation)
		static constexpr double omega = 0.7298;
		// local memory attraction (how much the particle wants to return to its best known position)
		static constexpr double phi_p = 1.55;
		// global memory attraction (how much the particle wants to go to swarm's best known position)
		static constexpr double phi_g = 2.05;
		// maximum velocity factor (the maximum length of velocity vector in fractions of search space)
		static constexpr double max_velocity = 0.4;

		// global memory repulsion (how much the particle wants to go away from global repulsors)
		static constexpr double phi_r = 3.05;
		// how many repulsory iterations should be at least done
		static constexpr size_t min_repulsory_iterations = 5;
		// ratio of repulsory iterations to problem size (e.g.; if this number is 0.5 and problem size is 20, a total of 10 iteations will be done)
		static constexpr double repulsory_itr_param_ratio = 0.25;

	protected:
		const solver::TSolver_Setup mSetup;
		const TUsed_Solution mLower_Bound;
		const TUsed_Solution mUpper_Bound;
		const TUsed_Solution mVelocity_Lower_Bound, mVelocity_Upper_Bound;
		pso::TSwarm_Vector<TUsed_Solution> mSwarm;
		pso::TSwarm_Vector<TUsed_Solution> mRepulsors;

		typename pso::TSwarm_Vector<TUsed_Solution>::iterator mBest_Itr;
		solver::TFitness mBest_Fitness = solver::Nan_Fitness;

		TUsed_Solution mOverall_Best;
		solver::TFitness mOverall_Best_Fitness = solver::Nan_Fitness;

		// updates global memory with best known position of the whole swarm
		inline void Update_Best_Candidate() {
			mBest_Itr = std::min_element(mSwarm.begin(), mSwarm.end(), [&](const pso::TCandidate_Solution<TUsed_Solution>& a, const pso::TCandidate_Solution<TUsed_Solution>& b) {
				return Compare_Solutions(a.current_fitness, b.current_fitness, mSetup.objectives_count, NFitness_Strategy::Master);
			});

			mBest_Fitness = mBest_Itr->best_fitness;

			if (Compare_Solutions(mBest_Fitness, mOverall_Best_Fitness, mSetup.objectives_count, NFitness_Strategy::Master))
			{
				mOverall_Best = mBest_Itr->best;
				mOverall_Best_Fitness = mBest_Fitness;
			}
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
			Init_PSO();
		}

		void Init_PSO() {
			mSwarm.resize(std::max(mSetup.population_size, static_cast<decltype(mSetup.population_size)>(5)));

			// create the initial swarm using supplied hints; fill up to a half of a swarm with hints
			const size_t initialized_count = std::min(mSwarm.size() / 2, mSetup.hint_count);

			for (size_t i = 0; i < initialized_count; i++)
				mSwarm[i].current = Vector_2_Solution<TUsed_Solution>(mSetup.hints[i], mSetup.problem_size);

			// complement the rest with generated candidates
			TSwarm_Generator<TUsed_Solution, TRandom_Device> gen;
			gen.Generate_Swarm(mSwarm, initialized_count, mSwarm.size(), mSetup.problem_size, mLower_Bound, mUpper_Bound);

			// calculate initial fitness
			std::for_each(std::execution::par_unseq, mSwarm.begin(), mSwarm.end(), [this](auto& candidate_solution) {
				if (mSetup.objective(mSetup.data, 1, candidate_solution.current.data(), candidate_solution.current_fitness.data()) != TRUE)
				{
					for (auto& elem : candidate_solution.current_fitness)
						elem = std::numeric_limits<double>::quiet_NaN();	// sanitize on error
				}

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

			size_t repulsory_iterations = repulsory ? std::max(static_cast<size_t>(repulsory_itr_param_ratio * mSetup.problem_size) + 1, min_repulsory_iterations) : 1;
			if constexpr (repulsory)
				progress.max_progress *= repulsory_iterations;

			size_t cur_progress = 0;
	
			const size_t solution_size = mSwarm[0].current.cols();
			TVelocity_Modifier<TUsed_Solution, TRandom_Device> mod;

			while ((cur_progress++ < mSetup.max_generations && repulsory_iterations > 0) && (progress.cancelled == FALSE))
			{
				progress.current_progress++;

				if constexpr (repulsory)
				{
					if (cur_progress == mSetup.max_generations)
					{
						cur_progress = 0;
						repulsory_iterations--;

						mRepulsors.push_back(*mBest_Itr);

						Init_PSO();
					}
				}

				// update the progress
				progress.best_metric = mOverall_Best_Fitness;

				std::for_each(std::execution::par_unseq, mSwarm.begin(), mSwarm.end(), [=, &mod](auto &candidate_solution) {

					// generate velocity update vectors/scalars (depends on chosen velocity update operator)
					auto [r_p, r_g] = mod.Generate_Velocity_Change(solution_size);

					if constexpr (repulsory)
					{
						// update velocity
						candidate_solution.velocity = 
							omega * candidate_solution.velocity
							+ phi_p * r_p * (candidate_solution.best - candidate_solution.current)
							+ phi_g * r_g * (mBest_Itr->best - candidate_solution.current);

						if (repulsory_iterations > 0)
						{
							// repulse (stronger in each repulsory iteration)
							for (auto& repulsor : mRepulsors)
								//candidate_solution.velocity -= phi_r * 1.0 / (repulsor.best - candidate_solution.current);
								//candidate_solution.velocity -= phi_r * (repulsor.best - candidate_solution.current) * (1.0 / (repulsor.best_fitness * candidate_solution.best_fitness)) / std::sqrt((repulsor.best - candidate_solution.current).pow(2).sum());
								candidate_solution.velocity -= phi_r * r_g * (repulsor.best - candidate_solution.current) / std::pow((repulsor.best - candidate_solution.current).pow(static_cast<double>(mRepulsors.size())).sum(), 1.0 / static_cast<double>(mRepulsors.size()));
								//candidate_solution.velocity -= phi_r * (repulsor.best - candidate_solution.current) * (repulsor.best_fitness / candidate_solution.best_fitness) / std::sqrt((repulsor.best - candidate_solution.current).pow(2).sum());
						}
						else
						{
							// attract in last iteration - take all repulsors and make them attractors (so they now become solution hints)
							for (auto& repulsor : mRepulsors)
								candidate_solution.velocity += phi_r * r_g * (repulsor.best - candidate_solution.current);
						}

						// ensure bounds
						candidate_solution.velocity = mVelocity_Upper_Bound.min(mVelocity_Lower_Bound.max(candidate_solution.velocity));
					}
					else
					{
						// update velocity and ensure bounds
						candidate_solution.velocity = mVelocity_Upper_Bound.min(mVelocity_Lower_Bound.max(
							omega * candidate_solution.velocity
							+ phi_p * r_p * (candidate_solution.best - candidate_solution.current)
							+ phi_g * r_g * (mBest_Itr->best - candidate_solution.current)
						));
					}

					// move particle by its velocity vector and ensure bounds
					candidate_solution.current = mUpper_Bound.min(mLower_Bound.max(candidate_solution.current + candidate_solution.velocity));

					// evaluate candidate solution
					if (mSetup.objective(mSetup.data, 1, candidate_solution.current.data(), candidate_solution.current_fitness.data()) != TRUE)
					{
						for (auto& elem : candidate_solution.current_fitness)
							elem = std::numeric_limits<double>::quiet_NaN();	// sanitize on error
					}

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

			return mOverall_Best;
		}
};
