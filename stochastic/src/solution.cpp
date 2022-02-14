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

bool Compare_Solutions(const double* a, const double* b, const size_t objectives_count, const bool strict_domination) {

	auto internal_compare = [](const double a, const double b)->bool {
		if (std::isnan(a))
			return false;

		if (std::isnan(b))
			return true;

		return a < b;	//less value is better	
	};

	//1. handle the special-case of the single objective
	if (objectives_count == 1) 
		return internal_compare(a[0], b[0]);

	//2. multi-objective => let's try to determine more dominating solution
	size_t a_dominations = 0, b_dominations = 0;
	for (size_t i = 0; i < objectives_count; i++) {
		if (internal_compare(a[i], b[i])) a_dominations++;				
		else if (internal_compare(b[i], a[i])) b_dominations++;
	}

	if (strict_domination) {
		return (a_dominations > 0) && (b_dominations == 0);
	}


	if (a_dominations > b_dominations)
		return true;


	if (b_dominations > a_dominations)
		return false;

	double a_accu = 0.0;
	double b_accu = 0.0;

	
	for (size_t i = 0; i < objectives_count; i++) {
		if (a[i] != b[i]) {
			//const double w = static_cast<double>(objectives_count - i);

			a_accu += a[i] * a[i];// *w;
			b_accu += b[i] * b[i];// *w;
		}
	}

	if (a_accu != b_accu)
		return a_accu < b_accu;
		
	//3. both a and b have equal number of dominations => cannot decide
	//	 and cannot compute a composite metric, as each metric could have different unit
	//	 => we assume that metrics are sorted by their prioirty and hce we compare them one by one
	for (size_t i = 0; i < objectives_count; i++) {
		if (a[i] != b[i]) 
			return internal_compare(a[i], b[i]);		
	}
	

	//4. they look equal, hence a does not dominate b
	return false;
}