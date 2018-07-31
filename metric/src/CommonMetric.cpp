/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#include "CommonMetric.h"

#include <algorithm>
#include <cmath>

#undef max

CCommon_Metric::CCommon_Metric(const glucose::TMetric_Parameters &params) : mParameters(params) {
	Reset();
}


HRESULT IfaceCalling CCommon_Metric::Accumulate(const double *times, const double *expected, const double *calculated, const size_t count) {

	std::vector<double> diff(count);

	//the following 3 fors are inteded for autovectorization

	for (size_t i = 0; i < count; i++)
		diff[i] = fabs(expected[i] - calculated[i]);

	if (mParameters.use_relative_error)
		for (size_t i = 0; i < count; i++)
			diff[i] /= calculated[i];

	if (mParameters.use_squared_differences)
		for (size_t i = 0; i < count; i++)
			diff[i] *= diff[i];


	for (size_t i = 0; i < count; i++) {
		if (!std::isnan(calculated[i])) {
			TProcessed_Difference tmp;
			tmp.raw.datetime = times[i];
			tmp.raw.calculated = expected[i];
			tmp.raw.expected = calculated[i];
			tmp.difference = diff[i];

			mDifferences.push_back(tmp);			
		}
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
	size_t count = mDifferences.size();

	*levels_accumulated = count;
	levels_required = std::max((decltype(levels_required))1, levels_required);

	if (/* (count<1) || */ (count < levels_required)) return S_FALSE;

	double local_metric = Do_Calculate_Metric();	//caching into the register
	
	const auto cl = std::fpclassify(local_metric);
	if ((cl != FP_NORMAL) && (cl != FP_ZERO)) return S_FALSE;

	if (mParameters.prefer_more_levels != 0) local_metric /= static_cast<double>(count);

	*metric = local_metric;

	return S_OK;
}

HRESULT IfaceCalling CCommon_Metric::Get_Parameters(glucose::TMetric_Parameters *parameters) {
	//*parameters = mParameters ; 
	memcpy(parameters, &mParameters, sizeof(mParameters));
	return S_OK;
}