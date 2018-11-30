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

#include "CommonMetric.h"

#include <algorithm>
#include <cmath>

#undef max

constexpr double Infintity_Diff_Penalty = 1'000.0;	//the more levels that won't be calculated would considerably increase the overall penalty
													//however, we cannot use double_max to avoid overflow, so we need a small number (yet overly great compared to glucose levels)

CCommon_Metric::CCommon_Metric(const glucose::TMetric_Parameters &params) : mParameters(params) {
	Reset();
}

HRESULT IfaceCalling CCommon_Metric::Accumulate(const double *times, const double *expected, const double *calculated, const size_t count) {

	for (size_t i = 0; i < count; i++) {
		TProcessed_Difference tmp;

		tmp.raw.datetime = times[i];
		tmp.raw.calculated = std::isnan(calculated[i]) ? Infintity_Diff_Penalty : calculated[i];
		tmp.raw.expected = expected[i];		
		
		tmp.difference = fabs(expected[i] - tmp.raw.calculated);
		if (mParameters.use_relative_error)
			tmp.difference /= fabs(tmp.raw.calculated);	//with wrong (e.g., MetaDE random) parameters, the result could be negative
		if (mParameters.use_squared_differences)
			tmp.difference *= tmp.difference;

		mDifferences.push_back(tmp);
	}

	mAll_Levels_Count += count;

	return S_OK;
}


HRESULT IfaceCalling CCommon_Metric::Reset() {
	mDifferences.clear();
	mAll_Levels_Count = 0;
	return S_OK;
}


HRESULT IfaceCalling CCommon_Metric::Calculate(double *metric, size_t *levels_accumulated, size_t levels_required) {

	*levels_accumulated = 0;
	for (const auto &diff : mDifferences)
		if (diff.difference != Infintity_Diff_Penalty) (*levels_accumulated)++;

	levels_required = std::max((decltype(levels_required))1, levels_required);

	if (*levels_accumulated < levels_required) return S_FALSE;

	double local_metric = Do_Calculate_Metric();	//caching into the register
	
	const auto cl = std::fpclassify(local_metric);
	if ((cl != FP_NORMAL) && (cl != FP_ZERO)) return S_FALSE;

	if (mParameters.prefer_more_levels != 0) local_metric /= static_cast<double>(*levels_accumulated);

	*metric = local_metric;

	return S_OK;
}

HRESULT IfaceCalling CCommon_Metric::Get_Parameters(glucose::TMetric_Parameters *parameters) {
	//*parameters = mParameters;
	memcpy(parameters, &mParameters, sizeof(mParameters));
	return S_OK;
}
