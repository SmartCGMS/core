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

#include "time_utils.h"

#include <scgms/utils/winapi_mapping.h>
#include <scgms/utils/string_utils.h>
#include <scgms/rtl/rattime.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

bool Is_Valid_Tm(std::tm& v) {
	// TODO: tweak this to truly validate result (i.e. day and month range, ..)
	return !(v.tm_mday == 0 || v.tm_mon < 0 || v.tm_year == 0);
}

bool Convert_Timestamp(std::string source, const char* sourceFormat, std::string &dest, const char* destFormat, time_t* unixTimeDst) {
	std::tm convtime = {};
	std::istringstream ss(source + " "); //we append extra space to avoid 1b) and enforce 1c) for https://en.cppreference.com/w/cpp/locale/time_get/get
										 //practically, if mask would e.g.; contain seconds, but the source would not, then the conversion would still succeed despite missing seconds
										 //the trailing, extra space removes the eof, thus making the conversion fail for the missing seconds

	if (ss >> std::get_time(&convtime, sourceFormat)) {
		if (!Is_Valid_Tm(convtime)) {
			return false;
		}

		if (destFormat) {
			std::stringstream timestr;
			timestr << std::put_time(&convtime, destFormat);

			dest = timestr.str();
		}

		if (unixTimeDst) {
			*unixTimeDst = mktime(&convtime);
		}

		return true;
	}
	else {
		return false;
	}
}

