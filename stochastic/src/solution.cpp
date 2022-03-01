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

#include "solution.h"

enum class partial_ordering
{
	unordered,
	less,
	greater
};

static inline partial_ordering Compare_Values(const double a, const double b)
{
	if (std::isnan(a) || std::isnan(b))
		return partial_ordering::unordered;

	if (a < b)
		return partial_ordering::less;
	if (a > b)
		return partial_ordering::greater;

	return partial_ordering::unordered;
}


bool Compare_Elements(const double a, const double b) {
	if (std::isnan(a))
		return false;

	if (std::isnan(b))
		return true;

	return a < b;	//less value is better	
}

std::tuple<size_t, size_t> Count_Dominance(const solver::TFitness& a, const solver::TFitness& b, const size_t objectives_count) {
	size_t a_count = 0;
	size_t b_count = 0;

	for (size_t i = 0; i < objectives_count; i++) {
		if (Compare_Elements(a[i], b[i])) a_count++;
		else if (Compare_Elements(b[i], a[i])) b_count++;
	}


	return std::tuple<size_t, size_t>{a_count, b_count};
}


template <bool weighted>
partial_ordering Euclidean_Distance(const solver::TFitness& a, const solver::TFitness& b, const size_t objectives_count) {
	double a_accu = 0.0;
	double b_accu = 0.0;

	for (size_t i = 0; i < objectives_count; i++) {
		if (a[i] != b[i]) {
			double a_tmp = a[i] * a[i];
			double b_tmp = b[i] * b[i];

			if constexpr (weighted) {
				const double w = static_cast<double>(objectives_count - i);
				a_tmp *= w;
				b_tmp *= w;
			}

			a_accu += a_tmp;
			b_accu += b_tmp;

		}
	}

	return Compare_Values(a_accu, b_accu); // a_accu <=> b_accu (as soon as C++20 is supported on all major platforms)
}



template <bool weighted>
partial_ordering Ratio_Distance(const solver::TFitness& a, const solver::TFitness& b, const size_t objectives_count) {
	double a_accu = 0.0;
	double b_accu = 0.0;

	for (size_t i = 0; i < objectives_count; i++) {
		if (a[i] != b[i]) {
			const double sum = a[i] + b[i];

			double a_tmp = a[i] / sum;
			double b_tmp = 1.0 - a_tmp;

			a_tmp *= a_tmp;
			b_tmp *= b_tmp;

			if constexpr (weighted) {
				const double w = static_cast<double>(objectives_count - i);
				a_tmp *= w;
				b_tmp *= w;
			}

			a_accu += a_tmp;
			b_accu += b_tmp;

		}
	}

	return Compare_Values(a_accu, b_accu); // a_accu <=> b_accu (as soon as C++20 is supported on all major platforms)
}


bool Compare_Solutions(const solver::TFitness& a, const solver::TFitness& b, const size_t objectives_count, const NFitness_Strategy strategy) {

	//1. handle the special-case of the single objective
	if (objectives_count == 1) 
		return Compare_Elements(a[0], b[0]);


	//2. multi-objective => let's try to determine more dominating solution
	if (strategy < NFitness_Strategy::Dominance_Count) {
		const auto [a_count, b_count] = Count_Dominance(a, b, objectives_count);

		if (a_count == 0)
			return false;	//solutions are either equal, or b strictly dominates a => in both cases, we return false

		if ((a_count >0) && (b_count == 0))
			return true;						//a strictly dominates b

		if (strategy == NFitness_Strategy::Strict_Dominance)
			return false;


		if (strategy == NFitness_Strategy::Soft_Dominance)
			return a_count > b_count;	//a is not dominated by b, not be is dominated by a - they are just two different, non-dominated solutions on the known Pareto front

		if (strategy == NFitness_Strategy::Any_Non_Dominated)
			return a_count > 0;			//just like the soft dominance; in both cases we increase the diversity on the best known Pareto front
		
		//at this point, the chosen dominance strategy takes another option to decide
	}
	

	partial_ordering comparison = partial_ordering::unordered;
	//3. let us decide by another option than the dominance
	switch (strategy) {
		

		case NFitness_Strategy::Weighted_Euclidean_Dominance: [[fallthrough]];
		case NFitness_Strategy::Weighted_Euclidean_Distance: 
			comparison = Euclidean_Distance<true>(a, b, objectives_count);
			break;


		case NFitness_Strategy::Ratio_Dominance:
			comparison = Ratio_Distance<false>(a, b, objectives_count);
			break;

		case NFitness_Strategy::Weighted_Ratio_Dominance:
			comparison = Ratio_Distance<true>(a, b, objectives_count);
			break;
		
		
		case NFitness_Strategy::Euclidean_Dominance: [[fallthrough]];
			//case NFitness_Strategy::Master: [[fallthrough]];
		case NFitness_Strategy::Euclidean_Distance: [[fallthrough]];
		default:
			comparison = Euclidean_Distance<false>(a, b, objectives_count);
			break;
		
	}


	//4. Is the comparison clearly decided?
	if (comparison == partial_ordering::less)
		return true;

	if (comparison == partial_ordering::greater)
		return false;

	
	//5. No, there's either some nan or the solutions could have exactly the same distance from the origin, but on different coordinates like [0, 1] and [1, 0] being equal in e2
	// => we have to compare by the metrics priority
	for (size_t i = 0; i < objectives_count; i++) {
		if (a[i] != b[i]) {
			if (Compare_Elements(a[i], b[i]))		//unlike the spaceship operator, this func considers the NaN in the evolutionary manner, in which a normal number should replace NaN
				return true;
		}
	}



	//4. they look equal, hence a does not dominate b
	return false;
}