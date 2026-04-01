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

#include <cstdint>
#include <string>
#include <vector>

class CRandom_Playback {
public:
	using result_type = uint64_t;
protected:
	const std::vector<result_type> mNumbers;
	size_t mCurrent_Number_Index = 0;
	size_t mRecent_Number_Index = 0;
public:
	CRandom_Playback();
	explicit CRandom_Playback(const std::string& token) : CRandom_Playback() {};
	CRandom_Playback(const CRandom_Playback&) = delete;

	CRandom_Playback operator=(const CRandom_Playback& other) = delete;

	result_type operator()();

	static constexpr result_type min() {
		return 0;
	}

	static constexpr result_type max() {
		return std::numeric_limits<result_type>::max();
	}
};
