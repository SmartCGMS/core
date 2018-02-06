/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#include "CommonMetric.h"

#include <algorithm>

#undef max

CCommon_Metric::CCommon_Metric(const glucose::TMetric_Parameters &params) : mParameters(params) {
	Reset();
}


void CCommon_Metric::Process_Differences() {
	for (auto &diff : mDifferences) {
		double tmp = diff.raw.expected - diff.raw.calculated;

		if (mParameters.use_relative_error) tmp /= diff.raw.expected;
		if (mParameters.use_squared_differences) tmp *= tmp;
		else tmp = fabs(tmp);

		diff.difference = tmp;
	}
}


HRESULT IfaceCalling CCommon_Metric::Accumulate(const glucose::TDifference_Point *begin, const glucose::TDifference_Point *end, size_t all_levels_count) {	

	for (auto iter = begin; iter < end; iter++) {
		TProcessed_Difference tmp;
		tmp.raw = *iter;
		mDifferences.push_back(tmp);
	}

	mAll_Levels_Count += all_levels_count;

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

	Process_Differences();	
	
	double local_metric = Do_Calculate_Metric();	//caching into the register
	
	const auto cl = fpclassify(local_metric);
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