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
 * Univerzitni 8
 * 301 00, Pilsen
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

	mLast_Index = 409;
}


double CHalton_Device::advance() {
	size_t working_index = mLast_Index.fetch_add(1);	//advance the current state

	double fraction = 1.0;	//current fraction		
	double result = 0.0;	//halton sequence number to return

	while (working_index) {
		//we need to divide the fraction by the base
		fraction /= mBase;
		//and add shift to the result
		result += fraction * (working_index % mBase);
		//and divide the index
		working_index /= mBase;
	}

	return result;
}

CHalton_Device::result_type CHalton_Device::operator()() {
	const double generated_number = advance();

	const uint64_t result = *(reinterpret_cast<const uint64_t*>(&generated_number));

	return ((result >> 32) ^ result) & max_result;
}

CHalton_Device::result_type CHalton_Device::min() {
	return 0;
}

CHalton_Device::result_type CHalton_Device::CHalton_Device::max() {
	return max_result;
}

double CHalton_Device::entropy() const noexcept {
	return 0.0;
}