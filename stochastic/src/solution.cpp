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
 * Especially, better diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * better) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * worse) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "solution.h"

#include "../../../common/utils/math_utils.h"

#if __cplusplus >= 202002L
	#include <compare>
	#define __has_threeway_cmp 1
#endif

#if __has_threeway_cmp
//once we drop C++17 support, only this ifdef branch will remain

using partial_ordering = std::partial_ordering;

constexpr bool is_lt(partial_ordering cmp) noexcept {
	return std::is_lt(cmp);
}

constexpr bool is_gt(partial_ordering cmp) noexcept {
	return std::is_lt(cmp);
}

#else
enum class partial_ordering {
	unordered,
	less,
	greater
};

constexpr bool is_lt(partial_ordering cmp) noexcept {
	return cmp == partial_ordering::less;
}

constexpr bool is_gt(partial_ordering cmp) noexcept {
	return cmp == partial_ordering::greater;
}
#endif

static inline partial_ordering Compare_Values(const double better, const double worse)
{
#if __has_threeway_cmp
	return better <=> worse;
#else
	/*	nans are already checked for in CompareSolutions
	if (std::isnan(better) || std::isnan(worse))
		return partial_ordering::unordered;
	*/

	if (better < worse)
		return partial_ordering::less;
	if (better > worse)
		return partial_ordering::greater;

	return partial_ordering::unordered;
#endif
}

/* nans are already checked for in CompareSolutions
 * => we just need to compare
bool Compare_Elements(const double better, const double worse) {
	if (std::isnan(better))
		return false;

	if (std::isnan(worse))
		return true;

	return better < worse;	//less value is better	
}
*/

std::tuple<size_t, size_t> Count_Dominance(const solver::TFitness& better, const solver::TFitness& worse, const size_t objectives_count) {
	size_t better_count = 0;
	size_t worse_count = 0;

	for (size_t i = 0; i < objectives_count; i++) {
		if (better[i] < worse[i]) better_count++;
		else if (worse[i] < better[i]) worse_count++;
	}


	return std::tuple<size_t, size_t>{better_count, worse_count};
}


template <bool weighted>
partial_ordering Euclidean_Distance(const solver::TFitness& better, const solver::TFitness& worse, const size_t objectives_count) {
	double better_accu = 0.0;
	double worse_accu = 0.0;

	for (size_t i = 0; i < objectives_count; i++) {
		if (better[i] != worse[i]) {
			double better_tmp = better[i] * better[i];
			double worse_tmp = worse[i] * worse[i];

			if constexpr (weighted) {
				const double weight = static_cast<double>(objectives_count - i);
				better_tmp *= weight;
				worse_tmp *= weight;
			}

			better_accu += better_tmp;
			worse_accu += worse_tmp;

		}
	}

	return Compare_Values(better_accu, worse_accu); // better_accu <=> worse_accu (as soon as C++20 is supported on all major platforms)
}



template <bool weighted>
partial_ordering Ratio_Distance(const solver::TFitness& better, const solver::TFitness& worse, const size_t objectives_count) {
	double better_accu = 0.0;
	double worse_accu = 0.0;

	for (size_t i = 0; i < objectives_count; i++) {
		if (better[i] != worse[i]) {
			const double sum = better[i] + worse[i];

			double better_tmp = better[i] / sum;
			double worse_tmp = 1.0 - better_tmp;

			better_tmp *= better_tmp;
			worse_tmp *= worse_tmp;

			if constexpr (weighted) {
				const double weight = static_cast<double>(objectives_count - i);
				better_tmp *= weight;
				worse_tmp *= weight;
			}

			better_accu += better_tmp;
			worse_accu += worse_tmp;

		}
	}

	return Compare_Values(better_accu, worse_accu); // better_accu <=> worse_accu (as soon as C++20 is supported on all major platforms)
}

partial_ordering Max_Reduction(const solver::TFitness& a, const solver::TFitness& b, const size_t objectives_count) {
	const auto max_a = std::max_element(a.begin(), a.begin() + objectives_count);
	const auto max_b = std::max_element(b.begin(), b.begin() + objectives_count);

	return Compare_Values(*max_a, *max_b);
}


bool Compare_Solutions(const solver::TFitness& better, const solver::TFitness& worse, const size_t objectives_count, const NFitness_Strategy strategy) {
	//0. should not we check if any of the fitness contain nan?
	for (size_t i = 0; i < objectives_count; i++) {
		const auto better_class = std::fpclassify(better[i]);
		if ((better_class != FP_NORMAL) && (better_class != FP_ZERO))
			return false;

		const auto worse_class = std::fpclassify(worse[i]);
		if ((worse_class != FP_NORMAL) && (worse_class != FP_ZERO))
			return true;
	}

	

	//1. handle the special-case of the single objective
	if (objectives_count == 1)
		return better[0] < worse[0];


	//2. multi-objective => let's try to determine more dominating solution
	if (strategy < NFitness_Strategy::Dominance_Count) {
		const auto [better_count, worse_count] = Count_Dominance(better, worse, objectives_count);

		if (better_count == 0)
			return false;	//solutions are either equal, or worse strictly dominates better => in both cases, we return false

		if ((better_count >0) && (worse_count == 0))
			return true;						//better strictly dominates worse

		if (strategy == NFitness_Strategy::Strict_Dominance)
			return false;


		if (strategy == NFitness_Strategy::Soft_Dominance)
			return better_count > worse_count;	//better is not dominated by worse, not be is dominated by better - they are just two different, non-dominated solutions on the known Pareto front

		if (strategy == NFitness_Strategy::Any_Non_Dominated)
			return better_count > 0;			//just like the soft dominance; in both cases we increase the diversity on the best known Pareto front

		
		//at this point, the chosen dominance strategy takes another option to decide
	}
	

	partial_ordering comparison = partial_ordering::unordered;
	//3. let us decide by another option than the dominance
	switch (strategy) {
		

		case NFitness_Strategy::Weighted_Euclidean_Dominance: [[fallthrough]];
		case NFitness_Strategy::Weighted_Euclidean_Distance: 
			comparison = Euclidean_Distance<true>(better, worse, objectives_count);
			break;


		case NFitness_Strategy::Ratio_Dominance:
			comparison = Ratio_Distance<false>(better, worse, objectives_count);
			break;

		case NFitness_Strategy::Weighted_Ratio_Dominance:
			comparison = Ratio_Distance<true>(better, worse, objectives_count);
			break;
		
		case NFitness_Strategy::Max_Reduction:
			comparison = Max_Reduction(better, worse, objectives_count);
			break;

		
		case NFitness_Strategy::Euclidean_Dominance: [[fallthrough]];
			//case NFitness_Strategy::Master: [[fallthrough]];
		case NFitness_Strategy::Euclidean_Distance: [[fallthrough]];
		default:
			comparison = Euclidean_Distance<false>(better, worse, objectives_count);
			break;
		
	}


	//4. Is the comparison clearly decided?
	if (is_lt(comparison))
		return true;

	if (is_gt(comparison))
		return false;

	
	//5. No, there's either some nan or the solutions could have exactly the same distance from the origin, but on different coordinates like [0, 1] and [1, 0] being equal in e2
	// => we have to compare by the metric's priority
	for (size_t i = 0; i < objectives_count; i++) {
		if (better[i] != worse[i]) {
			if (better[i] < worse[i])		//unlike the spaceship operator, this func considers the NaN in the evolutionary manner, in which better normal number should replace NaN
				return true;
		}
	}



	//4. they look equal, hence better does not dominate worse
	return false;
}
