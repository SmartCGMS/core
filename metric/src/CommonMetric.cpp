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

#include "CommonMetric.h"

#include <algorithm>
#include <cmath>

#undef max

constexpr double Infinity_Diff_Penalty = 1'000.0;	//the more levels that won't be calculated would considerably increase the overall penalty
													//however, we cannot use double_max to avoid overflow, so we need a small number (yet overly great compared to glucose levels)

CCommon_Metric::CCommon_Metric(const scgms::TMetric_Parameters& params) : mParameters(params) {
	Reset();
}

HRESULT IfaceCalling CCommon_Metric::Accumulate(const double *times, const double *expected, const double *calculated, const size_t count) {

	for (size_t i = 0; i < count; i++) {		
		if (!std::isnan(calculated[i])) {
			TProcessed_Difference tmp;

			tmp.raw.datetime = times[i];			
			tmp.raw.calculated = calculated[i];			
			tmp.raw.expected = expected[i];

			tmp.difference = fabs(expected[i] - tmp.raw.calculated);

			auto fp = std::fpclassify(tmp.difference);
			if (fp == FP_SUBNORMAL) {
				tmp.difference = 0.0;
				fp = FP_ZERO;
			}

			if ((fp == FP_NORMAL) || (fp == FP_ZERO)) {

				if (mParameters.use_relative_error) {

					if (tmp.raw.calculated != 0.0)
						tmp.difference /= fabs(tmp.raw.expected);	//with wrong (e.g., MetaDE random) parameters, the result could be negative
																	//albeit it is no longer a valid metric then (the triangle inequality wouldn't hold)
					else if (tmp.difference <= std::numeric_limits<double>::epsilon())
						tmp.difference = 0.0;		//if the difference is zero, then the relative difference is zero as well
					else
						tmp.difference = Infinity_Diff_Penalty;		//open case, what to do in such a case when we cannot divide by zero
				}
				if (mParameters.use_squared_differences)
					tmp.difference *= tmp.difference;
			}
			else
				tmp.difference = Infinity_Diff_Penalty;

			mDifferences.push_back(tmp);
		}
	}

	return S_OK;
}


HRESULT IfaceCalling CCommon_Metric::Reset() {
	mDifferences.clear();
	return S_OK;
}


HRESULT IfaceCalling CCommon_Metric::Calculate(double *metric, size_t *levels_accumulated, size_t levels_required) {

	if (levels_accumulated) {
		*levels_accumulated = mDifferences.size();
		levels_required = std::max((decltype(levels_required))1, levels_required);

		if (*levels_accumulated < levels_required) return S_FALSE;
	}

	double local_metric = Do_Calculate_Metric();	//caching into the register
	
	const auto cl = std::fpclassify(local_metric);
	if ((cl != FP_NORMAL) && (cl != FP_ZERO)) return S_FALSE;

	if (mParameters.prefer_more_levels != 0) local_metric /= static_cast<double>(mDifferences.size());

	*metric = local_metric;

	return S_OK;
}

HRESULT IfaceCalling CCommon_Metric::Get_Parameters(scgms::TMetric_Parameters *parameters) {
	//*parameters = mParameters;
	memcpy(parameters, &mParameters, sizeof(mParameters));
	return S_OK;
}
