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

#include "population.h"

constexpr bool DebugOut = false;

namespace rumoropt
{
	enum class NRumor_Strategy {
		Skeptic,			// individual believe his own truth, but considers the opinion of others slightly
		Realist,			// individual tends to consider his own perception equally to the perception of others
		Naive,				// individual tends to believe others
		Gossiper,			// individual believes others, but adds his own "truth"

		count,				// marker for strategy update
		start = Skeptic,	// marker for strategy update
	};

	enum class NRumor_Source {
		Current,		// retain from the current truth from the incident individual
		Best,			// retain from the best known truth from the incident individual
		//Noise,			// retain solely from the noise; NOTE: disabled, as it's superceded by a lifetime (time-to-live) mechanism
		Noisy_Best,		// retain from the best known truth from the incident individual, but add noise

		count
	};

	enum class NRumor_Order {
		Random,			// candidates are shuffled randomly before rumoring
		Best_To_Rest,	// candidates are sorted, so the better candidate improve worse ones

		count
	};

	constexpr NRumor_Order Rumor_Order = NRumor_Order::Random; // TODO: decide, if this is the best strategy

	// an array of rumor strategy distribution
	static constexpr std::array<NRumor_Strategy, 10> Rumor_Strategy_Dist = {
		NRumor_Strategy::Skeptic,
		NRumor_Strategy::Realist,
		NRumor_Strategy::Realist,
		NRumor_Strategy::Realist,
		NRumor_Strategy::Realist,
		NRumor_Strategy::Naive,
		NRumor_Strategy::Naive,
		NRumor_Strategy::Naive,
		NRumor_Strategy::Gossiper,
		NRumor_Strategy::Gossiper
	};

	// size of knowledge pattern bank
	static constexpr size_t Bank_Size = 32;

	// probability of retaining pattern from bank
	static constexpr double p_Rumor_Bank = 0.6;
	// probability of retaining pattern from noise
	static constexpr double p_Rumor_Noise = 0.4;

	// ensure probability bounds
	static_assert(p_Rumor_Bank + p_Rumor_Noise <= 1.0, "Sum of rumor chances (Bank+Noise) must be less or equal 1");

	// rate of decay for bank items (every tick, this much of weight decays away)
	static constexpr double Bank_Decay = 0.05;

	// skepticist weight of belief
	static constexpr double w_Skeptic_start = 0.1;
	static constexpr double w_Skeptic_end = 0.3;
	// realists' weight of belief
	static constexpr double w_Realist_start = 0.4;
	static constexpr double w_Realist_end = 0.6;
	// naivists' weight of belief
	static constexpr double w_Naive_start = 0.8;
	static constexpr double w_Naive_end = 0.9;
	// gossiper's weight of belief
	static constexpr double w_Gossiper_start = 1.1;
	static constexpr double w_Gossiper_end = 1.4;

	// weight of noise added to the rumor
	static constexpr double w_Noise = 0.15;

	// minimum number of generations the individual should live
	static constexpr size_t Life_Expectancy_Min = 2;
	// maximum number of generations the individual should live (except the best one, which always survives)
	static constexpr size_t Life_Expectancy_Max = 100;

	// there will be this fraction of population count of incidences throughout a single rumor epoch
	static constexpr double f_Indicence_Cnt = 0.9;

	template <typename TUsed_Solution>
	struct TCandidate_Solution {
		// current parameter vector
		TUsed_Solution current;
		// next parameter vector
		TUsed_Solution next;
		// best parameter vector
		TUsed_Solution best;
		// current parameter vector fitness
		solver::TFitness current_fitness = solver::Nan_Fitness;
		// next parameter vector fitness
		solver::TFitness next_fitness = solver::Nan_Fitness;
		// best known parameter vector fitness
		solver::TFitness best_fitness = solver::Nan_Fitness;
		// remaining generation count; the element is discarded and overriden by a new one
		size_t life_counter;

		// rumor strategy of this individual
		NRumor_Strategy strategy;
		// currently generated rumor weight
		double rumor_weight;
	};

	template<typename TUsed_Solution>
	struct TSlice {
		// bitmap of rumored pieces
		TUsed_Solution rumored;
		// total improvement score
		double delta = 0.0;
	};

	// population vector used in calculations
	template<typename TUsed_Solution>
	using TPopulation_Vector = TAligned_Solution_Vector<rumoropt::TCandidate_Solution<TUsed_Solution>>;

	// vector of bank patterns
	template<typename TUsed_Solution>
	using TBank = std::array<TSlice<TUsed_Solution>, Bank_Size>;
}


template <typename TUsed_Solution, typename TRandom_Device = std::random_device>
class CRumor_Opt
{
	protected:
		const solver::TSolver_Setup mSetup;
		const TUsed_Solution mLower_Bound;
		const TUsed_Solution mUpper_Bound;
		const TUsed_Solution mVelocity_Lower_Bound, mVelocity_Upper_Bound;
		const TUsed_Solution mBounds_Range;
		rumoropt::TPopulation_Vector<TUsed_Solution> mPopulation;
		rumoropt::TBank<TUsed_Solution> mBank;

		typename rumoropt::TPopulation_Vector<TUsed_Solution>::iterator mBest_Itr;
		solver::TFitness mBest_Fitness = solver::Nan_Fitness;

		// updates global memory with best known "truth" of the whole population
		inline void Update_Best_Candidate() {
			mBest_Itr = std::min_element(mPopulation.begin(), mPopulation.end(), [&](const rumoropt::TCandidate_Solution<TUsed_Solution>& a, const rumoropt::TCandidate_Solution<TUsed_Solution>& b) {
				return Compare_Solutions(a.best_fitness, b.best_fitness, mSetup.objectives_count, NFitness_Strategy::Master);
			});

			mBest_Fitness = mBest_Itr->current_fitness;
		}

		inline size_t Gen_Random_Population_Idx(size_t startIdx) {
			return startIdx + static_cast<size_t>(static_cast<double>(mPopulation.size() - startIdx) * mUniform_Distribution_dbl(mRandom_Generator));
		}

	protected:
		inline static TRandom_Device mRandom_Generator;
		inline static thread_local std::uniform_real_distribution<double> mUniform_Distribution_dbl{ 0.0, 1.0 };
		inline static thread_local std::exponential_distribution<double> mExp_Distribution_dbl{ 4.0 };
		inline static thread_local std::uniform_int_distribution<size_t> mStrategy_Distribution{ 0, rumoropt::Rumor_Strategy_Dist.size() - 1 };
		inline static thread_local std::uniform_int_distribution<size_t> mSource_Distribution{ 0ULL, static_cast<size_t>(rumoropt::NRumor_Source::count) };
		inline static thread_local std::uniform_int_distribution<size_t> mLife_Distribution{ rumoropt::Life_Expectancy_Min, rumoropt::Life_Expectancy_Max };

		// a vector of update vectors and their fitness improvements
		std::vector<rumoropt::TSlice<TUsed_Solution>> mEpoch_Slice_Map;

	public:
		CRumor_Opt(const solver::TSolver_Setup &setup) :
			mSetup(solver::Check_Default_Parameters(setup, 1'000, 100)),
			mLower_Bound(Vector_2_Solution<TUsed_Solution>(setup.lower_bound, setup.problem_size)),
			mUpper_Bound(Vector_2_Solution<TUsed_Solution>(setup.upper_bound, setup.problem_size)),
			mBounds_Range(mUpper_Bound - mLower_Bound)
		{
			Init_Population();
		}

		void Init_Population() {
			mPopulation.resize(std::max(mSetup.population_size, static_cast<decltype(mSetup.population_size)>(5)));

			// create the initial population using supplied hints; fill up to a half of a population with hints
			const size_t initialized_count = std::min(mPopulation.size() / 2, mSetup.hint_count);

			// 1. create the initial population
			std::vector<TUsed_Solution> trimmed_hints;
			std::vector<size_t> hint_indexes(mSetup.hint_count);
			std::vector<solver::TFitness> hint_fitness(mSetup.hint_count, solver::Nan_Fitness);
			std::vector<bool> hint_validity(mSetup.hint_count);

			// a) find the best hints
			// trim the parameters to the bounds
			std::iota(std::begin(hint_indexes), std::end(hint_indexes), 0);
			for (size_t i = 0; i < mSetup.hint_count; i++) {
				trimmed_hints.push_back(mUpper_Bound.min(mLower_Bound.max(Vector_2_Solution<TUsed_Solution>(mSetup.hints[hint_indexes[i]], mSetup.problem_size))));//ensure the bounds
			}

			// b) check their fitness in parallel
			std::for_each(std::execution::par_unseq, hint_indexes.begin(), hint_indexes.end(), [this, &trimmed_hints, &hint_fitness, &hint_validity](auto& hint_idx) {
				hint_validity[hint_idx] = mSetup.objective(mSetup.data, 1, trimmed_hints[hint_idx].data(), hint_fitness[hint_idx].data()) == TRUE;
			});

			if (initialized_count < mSetup.hint_count) {
				// and sort the select up to the initialized_count best of them - if actually needed
				std::partial_sort(hint_indexes.begin(), hint_indexes.begin() + initialized_count, hint_indexes.end(),
					[&](const size_t& a, const size_t& b) {
						if (!hint_validity[hint_indexes[a]])
							return false;
						if (!hint_validity[hint_indexes[b]])
							return true;

						return Compare_Solutions(hint_fitness[hint_indexes[a]], hint_fitness[hint_indexes[b]], mSetup.objectives_count, NFitness_Strategy::Master);
						// true/false domination see the sorting in the main cycle
					});
			}

			// 1. create the initial population

			// b) by storing suggested params
			size_t effectively_initialized_count = 0;
			for (size_t i = 0; i < initialized_count; i++) {
				if (hint_validity[hint_indexes[i]]) {
					effectively_initialized_count++;
					mPopulation[i].current = trimmed_hints[hint_indexes[i]];
					mPopulation[i].current_fitness = hint_fitness[hint_indexes[i]];
				}
				else
					break;
			}

			// c) by complementing it with randomly generated numbers
			for (size_t i = effectively_initialized_count; i < mPopulation.size(); i++) {
				TUsed_Solution tmp;

				// this helps when we use generic dst vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
				// optimized away in compile time
				tmp.resize(Eigen::NoChange, mSetup.problem_size);

				for (size_t j = 0; j < mSetup.problem_size; j++)
					tmp[j] = mUniform_Distribution_dbl(mRandom_Generator);

				mPopulation[i].current = mLower_Bound + tmp.cwiseProduct(mBounds_Range);
				mPopulation[i].next = mPopulation[i].current;

				Generate_Rumor_Weight(i, true);
			}

			// compute the fitness in parallel
			std::for_each(std::execution::par_unseq, mPopulation.begin(), mPopulation.end(), [this](auto& candidate_solution) {
				if (mSetup.objective(mSetup.data, 1, candidate_solution.current.data(), candidate_solution.current_fitness.data()) != TRUE) {
					for (auto& elem : candidate_solution.current_fitness)
						elem = std::numeric_limits<double>::quiet_NaN();
				}

				candidate_solution.next = candidate_solution.current;
				candidate_solution.best = candidate_solution.current;
				candidate_solution.best_fitness = candidate_solution.current_fitness;

				candidate_solution.life_counter = mLife_Distribution(mRandom_Generator);
			});

			// generate random bank members with negative deltas (so they could be updated once any better improvement vector gets generated)
			std::for_each(std::execution::par_unseq, mBank.begin(), mBank.end(), [this](rumoropt::TSlice<TUsed_Solution>& slice) {
				
				slice.rumored.resize(Eigen::NoChange, mSetup.problem_size);

				for (size_t j = 0; j < mSetup.problem_size; j++)
					slice.rumored[j] = (mUniform_Distribution_dbl(mRandom_Generator) < 0.5) ? 0 : 1;

				slice.delta = -100000; // for the actual delta to be unknown, let's assign something significantly small
			});

			// sort the bank - best members first
			std::sort(std::execution::par_unseq, mBank.begin(), mBank.end(), [](const rumoropt::TSlice<TUsed_Solution>& a, const rumoropt::TSlice<TUsed_Solution>& b) {
				return a.delta > b.delta;
			});

			// select population best candidate and store it
			Update_Best_Candidate();
		}

		// generates a new rumor weight (and possibly a strategy) for a given candidate
		void Generate_Rumor_Weight(size_t idx, bool refreshStrategy) {
			Generate_Rumor_Weight(mPopulation.begin() + idx, refreshStrategy);
		}

		// generates a new rumor weight (and possibly a strategy) for a given candidate by its iterator
		void Generate_Rumor_Weight(typename rumoropt::TPopulation_Vector<TUsed_Solution>::iterator itr, bool refreshStrategy)
		{
			if (refreshStrategy)
				itr->strategy = rumoropt::Rumor_Strategy_Dist[mStrategy_Distribution(mRandom_Generator)]; // generate strategy according to pre-determined distribution

			switch (itr->strategy)
			{
				case rumoropt::NRumor_Strategy::Skeptic: itr->rumor_weight = rumoropt::w_Skeptic_start + mUniform_Distribution_dbl(mRandom_Generator) * (rumoropt::w_Skeptic_end - rumoropt::w_Skeptic_start); break;
				case rumoropt::NRumor_Strategy::Realist: itr->rumor_weight = rumoropt::w_Realist_start + mUniform_Distribution_dbl(mRandom_Generator) * (rumoropt::w_Realist_end - rumoropt::w_Realist_start); break;
				case rumoropt::NRumor_Strategy::Naive:   itr->rumor_weight = rumoropt::w_Naive_start + mUniform_Distribution_dbl(mRandom_Generator) * (rumoropt::w_Naive_end - rumoropt::w_Naive_start); break;
				case rumoropt::NRumor_Strategy::Gossiper:itr->rumor_weight = rumoropt::w_Gossiper_start + mUniform_Distribution_dbl(mRandom_Generator) * (rumoropt::w_Gossiper_end - rumoropt::w_Gossiper_start); break;
			}
		}

		/*
		// creates a new noise candidate
		void Create_Noise_Candidate() {
			mNoise.resize(Eigen::NoChange, mSetup.problem_size);
			for (size_t j = 0; j < mSetup.problem_size; j++)
				mNoise[j] = mUniform_Distribution_dbl(mRandom_Generator);

			mNoise = mLower_Bound + mNoise.cwiseProduct(mBounds_Range);
		}
		*/

		// performs rumoring between two candidate solutions - updates 'a' ("from b to a")
		void Rumor(rumoropt::TCandidate_Solution<TUsed_Solution>& a, const rumoropt::TCandidate_Solution<TUsed_Solution>& b, size_t incidenceIdx) {

			rumoropt::TSlice<TUsed_Solution>& slice = mEpoch_Slice_Map[incidenceIdx];
			slice.rumored.resize(Eigen::NoChange, mSetup.problem_size);

			mEpoch_Slice_Map[incidenceIdx].delta = 0;

			double rnd = mUniform_Distribution_dbl(mRandom_Generator);

			// a vector of changes retained from bank
			if (rnd < rumoropt::p_Rumor_Bank)
			{
				// use exponential distribution to prefer better bank members
				const size_t idx = std::min(static_cast<size_t>(mExp_Distribution_dbl(mRandom_Generator) * static_cast<double>(rumoropt::Bank_Size)), rumoropt::Bank_Size - 1);
				slice.rumored = mBank[idx].rumored;
			}
			// a vector of changes generated randomly
			else if (rnd < rumoropt::p_Rumor_Bank + rumoropt::p_Rumor_Noise)
			{
				for (size_t j = 0; j < mSetup.problem_size; j++)
					slice.rumored[j] = (mUniform_Distribution_dbl(mRandom_Generator) < 0.5) ? 0 : 1;
			}
			else
				return; // no rumoring

			// choose a rumor source
			rumoropt::NRumor_Source src = static_cast<rumoropt::NRumor_Source>(mSource_Distribution(mRandom_Generator));

			switch (src)
			{
				// current - take current "truth" from second candidate
				case rumoropt::NRumor_Source::Current:
				{
					a.next = b.current * a.rumor_weight * slice.rumored + b.current * (1 - slice.rumored) + a.current * (1 - a.rumor_weight) * slice.rumored;
					break;
				}
				// best - take best known "truth" from second candidate
				case rumoropt::NRumor_Source::Best:
				{
					a.next = b.best * a.rumor_weight * slice.rumored + b.best * (1 - slice.rumored) + a.current * (1 - a.rumor_weight) * slice.rumored;
					break;
				}
				// noise - take a randomly generated noise as source
				/*case rumoropt::NRumor_Source::Noise:
				{
					for (size_t i = 0; i < mSetup.problem_size; i++)
					{
						const double noise = mLower_Bound[i] + mUniform_Distribution_dbl(mRandom_Generator) * mBounds_Range[i];
						a.next[i] = noise * a.rumor_weight * slice.rumored[i] + noise * (1 - slice.rumored[i]) + a.current[i] * (1 - a.rumor_weight) * slice.rumored[i];
					}
					break;
				}*/
				// noisy best - take best known "truth" from second candidate and add a little bit of noise
				case rumoropt::NRumor_Source::Noisy_Best:
				{
					for (size_t i = 0; i < mSetup.problem_size; i++)
					{
						const double noise = mLower_Bound[i] + mUniform_Distribution_dbl(mRandom_Generator) * mBounds_Range[i];
						a.next[i] = (noise * rumoropt::w_Noise * slice.rumored[i] + b.best[i] * (1 - rumoropt::w_Noise)) * a.rumor_weight * slice.rumored[i] + b.best[i] * (1 - slice.rumored[i]) + a.current[i] * (1 - a.rumor_weight) * slice.rumored[i];
					}
					break;
				}
			}

			// update "next" candidate vector
			a.next = mUpper_Bound.min(mLower_Bound.max(a.next));
		}

		// update bank according to used slice (from epoch map)
		void Update_Bank(const rumoropt::TSlice<TUsed_Solution>& slice) {

			// update bank only if the rumored slice is not all zeroes
			if (slice.rumored.isZero())
				return;

			size_t sliceIdx = static_cast<size_t>(-1);

			// find existing slice
			for (size_t i = 0; i < rumoropt::Bank_Size - 1; i++)
			{
				if ((slice.rumored == mBank[i].rumored).all())
				{
					sliceIdx = i;
					break;
				}
			}

			// slice already exists - update weight
			if (sliceIdx != static_cast<size_t>(-1))
			{
				// automatically accept globally improving instances
				if (mBank[sliceIdx].delta < 0 && slice.delta > 0)
					mBank[sliceIdx].delta = slice.delta;
				// accept locally improving instances
				else if (slice.delta > mBank[sliceIdx].delta)
					mBank[sliceIdx].delta = (mBank[sliceIdx].delta + slice.delta) / 2.0; // TODO: a better strategy?
				// discard worsening instances
				// TODO: is this right?
			}
			else // slice does not exist - select the weakest bank slice and replace it, if the newly discovered slice performs better
			{
				for (size_t i = rumoropt::Bank_Size - 1; i > 0; i--)
				{
					if (mBank[i].delta < slice.delta)
					{
						mBank[i].rumored = slice.rumored;
						mBank[i].delta = slice.delta;
						break;
					}
				}
			}
		}

		TUsed_Solution Solve(solver::TSolver_Progress &progress) {

			population_helpers::CPopulation_Dump dumper(mSetup.problem_size);

			if constexpr (DebugOut)
			{
				dumper.Init("rumor_dump.csv");
			}

			progress.current_progress = 0;
			progress.max_progress = mSetup.max_generations;

			size_t cur_progress = 0;
	
			const size_t solution_size = mPopulation[0].current.cols();
			const size_t incidence_per_epoch = static_cast<size_t>(rumoropt::f_Indicence_Cnt * static_cast<double>(mPopulation.size()));

			// prepare epoch slice map
			mEpoch_Slice_Map.resize(incidence_per_epoch);
			for (size_t i = 0; i < incidence_per_epoch; i++)
				mEpoch_Slice_Map[i].rumored.resize(Eigen::NoChange, mSetup.problem_size);

			// generate population indices vector in order to be able to perform parallel population update whilst having all indices
			std::vector<size_t> mPop_Idx(mPopulation.size());
			std::iota(mPop_Idx.begin(), mPop_Idx.end(), 0);

			if constexpr (DebugOut)
			{
				dumper.Dump(cur_progress, mPopulation, [](const rumoropt::TCandidate_Solution<TUsed_Solution>& solution) {
					return solution.current_fitness[0];
				}, [](const rumoropt::TCandidate_Solution<TUsed_Solution>& solution, size_t paramIdx) {
					return solution.current[paramIdx];
				});
			}

			while (cur_progress++ < mSetup.max_generations && (progress.cancelled == FALSE))
			{
				if constexpr (rumoropt::Rumor_Order == rumoropt::NRumor_Order::Random)
				{
					// shuffle the population at each epoch, so the incidence is different from epoch to epoch
					std::shuffle(mPopulation.begin(), mPopulation.end(), mRandom_Generator);
				}
				else if constexpr (rumoropt::Rumor_Order == rumoropt::NRumor_Order::Best_To_Rest)
				{
					// update worse candidates with better ones
					std::sort(mPopulation.begin(), mPopulation.end(), [this](const rumoropt::TCandidate_Solution<TUsed_Solution>& a, const rumoropt::TCandidate_Solution<TUsed_Solution>& b) {
						return !Compare_Solutions(a.current_fitness, b.current_fitness, mSetup.objectives_count, NFitness_Strategy::Master);
					});
				}

				// update the progress
				progress.current_progress++;
				progress.best_metric = mBest_Fitness;

				// 1) rumors, recalculate objective function, update best solution
				std::for_each(std::execution::par_unseq, mPop_Idx.begin(), mPop_Idx.begin() + incidence_per_epoch, [=](auto candidate_solution_idx) {

					auto& candidate_solution = mPopulation[candidate_solution_idx];

					// generate incidence
					Rumor(mPopulation[candidate_solution_idx], mPopulation[Gen_Random_Population_Idx(candidate_solution_idx + 1)], candidate_solution_idx);

					if (mSetup.objective(mSetup.data, 1, candidate_solution.next.data(), candidate_solution.next_fitness.data()) == TRUE)
					{
						// store incidence delta
						mEpoch_Slice_Map[candidate_solution_idx].delta = candidate_solution.next_fitness[0] - candidate_solution.current_fitness[0];
						if (std::isnan(mEpoch_Slice_Map[candidate_solution_idx].delta))
							mEpoch_Slice_Map[candidate_solution_idx].delta = candidate_solution.next_fitness[0]; // TODO: solve this better

						// if the newly generated solution is better, use it; otherwise discard it
						if (Compare_Solutions(candidate_solution.next_fitness, candidate_solution.current_fitness, mSetup.objectives_count, NFitness_Strategy::Master))
						{
							candidate_solution.current = candidate_solution.next;
							candidate_solution.current_fitness = candidate_solution.next_fitness;
						}
						else
						{
							//candidate_solution.next = candidate_solution.current;
							//candidate_solution.next_fitness = candidate_solution.current_fitness;
						}
					}
					else
					{
						//candidate_solution.next = candidate_solution.current;
						//candidate_solution.next_fitness = candidate_solution.current_fitness;
					}

					// did we generate a solution, that is better than current best? If yes, update best known "truth"
					if (Compare_Solutions(candidate_solution.current_fitness, candidate_solution.best_fitness, mSetup.objectives_count, NFitness_Strategy::Master))
					{
						candidate_solution.best = candidate_solution.current;
						candidate_solution.best_fitness = candidate_solution.current_fitness;
					}
				});

				// 2) decay bank - this allows for less improving update vectors to become more visible to the rumoring process, if they are still improving the candidates
				for (size_t i = 0; i < rumoropt::Bank_Size; i++)
					mBank[i].delta = (1.0 - rumoropt::Bank_Decay) * mBank[i].delta;

				// 3) update bank - update with new improvement data
				for (size_t i = 0; i < incidence_per_epoch; i++)
					Update_Bank(mEpoch_Slice_Map[i]);

				// 4) sort the bank - best bank members first
				std::sort(std::execution::par_unseq, mBank.begin(), mBank.end(), [](const rumoropt::TSlice<TUsed_Solution>& a, const rumoropt::TSlice<TUsed_Solution>& b) {
					return a.delta > b.delta;
				});

				if constexpr (DebugOut)
				{
					dprintf("*** Bank dump begin ***\r\n");
					for (size_t i = 0; i < rumoropt::Bank_Size; i++)
					{
						dprintf("Slice");
						for (size_t j = 0; j < mSetup.problem_size; j++)
							dprintf(" %d", static_cast<size_t>(mBank[i].rumored[j]));
						dprintf(", delta %llf\r\n", mBank[i].delta);
					}
					dprintf("*** Bank dump end ***\r\n");

					dumper.Dump(cur_progress, mPopulation, [](const rumoropt::TCandidate_Solution<TUsed_Solution>& solution) {
						return solution.current_fitness[0];
					}, [](const rumoropt::TCandidate_Solution<TUsed_Solution>& solution, size_t paramIdx) {
						return solution.current[paramIdx];
					});
				}

				// update population-best candidate
				Update_Best_Candidate();

				// regenerate too old candidate solutions (time-to-live)
				for (auto itr = mPopulation.begin(); itr != mPopulation.end(); ++itr)
				{
					// the best candidate may live past his time-to-live, until superceded by another
					if (itr != mBest_Itr)
					{
						itr->life_counter--;
						if (itr->life_counter == 0)
						{
							TUsed_Solution tmp;

							// this helps when we use generic dst vector, and does nothing when we use fixed lengths (since ColsAtCompileTime already equals bounds_range.cols(), so it gets
							// optimized away in compile time
							tmp.resize(Eigen::NoChange, mSetup.problem_size);

							for (size_t j = 0; j < mSetup.problem_size; j++)
								tmp[j] = mUniform_Distribution_dbl(mRandom_Generator);

							itr->current = mLower_Bound + tmp.cwiseProduct(mBounds_Range);
							itr->next = itr->current;
							itr->best = itr->current;

							if (mSetup.objective(mSetup.data, 1, itr->current.data(), itr->current_fitness.data()) != TRUE) {
								for (auto& elem : itr->current_fitness)
									elem = std::numeric_limits<double>::quiet_NaN();
							}

							itr->best_fitness = itr->current_fitness;

							Generate_Rumor_Weight(itr, true);

							itr->life_counter = mLife_Distribution(mRandom_Generator);
						}
					}
				}
			}

			return mBest_Itr->best;
		}
};
