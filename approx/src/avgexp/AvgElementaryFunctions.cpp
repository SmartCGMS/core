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

#include "AvgElementaryFunctions.h"

#include <cmath>
#include <cassert>

double ComputeExpK(const TGlucoseLevel& lpt, const TGlucoseLevel& rpt) {

	assert((rpt.level > 0.0) && (lpt.level > 0.0));

	double delta = rpt.datetime - lpt.datetime;
	double k;
	if (delta != 0.0) {
		k = std::log(rpt.level / lpt.level) / delta;
	}
	else {
		k = 0.0;	//seems like there are two levels at the same time
	}

	assert(!isnan(k));

	return k;
}

double ComputeExpKX(const TAvgExpPoint& lpt, const double rx) {
	double ry = lpt.pt.level*std::exp(lpt.k*(rx - lpt.pt.datetime));

	//double ry = (rx - lpt->pt.datetime)*lpt->k + lpt->pt.level;

	assert(!isnan(ry));

	return ry;
}

double ComputeLineK(const TGlucoseLevel& lpt, const TGlucoseLevel& rpt) {

	assert((rpt.level > 0.0) && (lpt.level > 0.0));
	
	double delta = rpt.datetime - lpt.datetime;
	double k;
	if (delta != 0) {
		k = (rpt.level - lpt.level) / delta;
	}
	else {
		k = 0;
	}

	return k;
}

double ComputeLineKX(const TAvgExpPoint& lpt, const double rx) {
	const double ry = lpt.k*(rx - lpt.pt.datetime) + lpt.pt.level;

	return ry;
}

const TAvgElementary AvgExpElementary = { -489.6, -489.0 , ComputeExpK, ComputeExpKX};
const TAvgElementary AvgLineElementary = { -489.6, -489.0, ComputeLineK, ComputeLineKX };
