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

#include "HaltonDevice.h"

std::atomic<size_t> last_prime;

size_t CHalton_Device::Get_Next_Prime(size_t last_number) {	
	if (!(last_number & 1)) last_number++;	//no need to test odd numbers

	bool is_prime;
	do {
		last_number += 2;
		is_prime = true;
		for (size_t c = 2; c*c <= last_number; c++)
			//if the last_number divides by c with zero reminder, then the last_number is not prime
			if (!(last_number % c)) {
				is_prime = false;
				break;
			}

	} while (!is_prime);

	return last_number;
}

CHalton_Device::CHalton_Device() {
	size_t current_prime;
	do {
		current_prime = last_prime.load();
		mBase = Get_Next_Prime(current_prime);		
	} while (!last_prime.compare_exchange_weak(current_prime, mBase));
	
	mInv_Base = 1.0 / static_cast<double>(mBase);
	mLast_Index = 409;
}


double CHalton_Device::advance() {

	lldiv_t qr;	//qr.quot is the working index
	qr.quot = mLast_Index.fetch_add(1);	//advance the current state	

	double fraction = 1.0;	//current fraction		
	double result = 0.0;	//halton sequence number to return

	
	while (qr.quot) {
		//we need to divide the fraction by the base
		fraction *= mInv_Base;

		//divide to got both quotient and reminder at once
		qr = std::lldiv(qr.quot, mBase);

		//and add shift to the result
		result += fraction * static_cast<double>(qr.rem);
	}


	return result;
}

CHalton_Device::result_type CHalton_Device::operator()() {
	const double generated_number = advance();

	// use union "casting" instead of reinterpret_cast to avoid strict aliasing rules violation by dereferencing type-punned pointer

	const union {
		double as_double;
		uint64_t as_uint64;
	} resultCast = { generated_number };

	//const uint64_t result = *(reinterpret_cast<const uint64_t*>(&generated_number));
	//return ((result >> 32) ^ result) & max_result;

	return ((resultCast.as_uint64 >> 32) ^ resultCast.as_uint64) & max_result;
}

double CHalton_Device::entropy() const noexcept {
	return 0.0;
}