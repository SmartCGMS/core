/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#include "CommonMetric.h"

#include <algorithm>

#undef max

CCommonDiffMetric::CCommonDiffMetric(TMetricParameters params) : mParameters(params) {
	Reset();
}


void CCommonDiffMetric::ProcessDifferencesAsDifferences() {
	size_t count = mDifferences.size();
	TProcessedDifference *diffs = mDifferences.data();	//this method will not be called with count<1

	for (size_t i = 0; i < count; i++) {
		floattype tmp = diffs[i].raw.expected - diffs[i].raw.calculated;

		if (mParameters.UseRelativeValues) tmp /= diffs[i].raw.expected;
		if (mParameters.UseSquaredDifferences) tmp *= tmp;
		else tmp = fabs(tmp);

		diffs[i].difference = tmp;
	}
}

void CCommonDiffMetric::ProcessDifferencesWithISO2013Tolerance() {
	size_t count = mDifferences.size();
	TProcessedDifference *diffs = mDifferences.data();	//this method will not be called with count<1

	const floattype bglowthreshold = 75 * 0.0555;	//75 and  mg/dL in mmol/l
	const floattype bglowtolerance = 15 * 0.0555;

	for (size_t i = 0; i < count; i++) {
		floattype absdiff = fabs(diffs[i].raw.expected - diffs[i].raw.calculated);

		floattype reldiff = absdiff / diffs[i].raw.expected;
		if ((reldiff <= 15.0) || ((diffs[i].raw.expected < bglowthreshold) && (absdiff <= bglowtolerance))) {
			absdiff = 0.0;
			reldiff = 0.0;
			diffs[i].raw.calculated = diffs[i].raw.expected;
		}


		if (mParameters.UseRelativeValues) absdiff = reldiff;
		if (mParameters.UseSquaredDifferences) absdiff *= absdiff;


		diffs[i].difference = absdiff;
	}
}

void CCommonDiffMetric::ProcessDifferencesAsSlopes() {
	size_t count = mDifferences.size();
	TProcessedDifference *diffs = mDifferences.data();	//this method will not be called with count<1

	//let us calculate slopes from n to n-1 points

	//1. we have to sort differences accordingly to the time
	sort(mDifferences.begin(), mDifferences.end(),
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.raw.datetime < b.raw.datetime; }
	);

	//2. for the first differences, the slope cannot be calculated at all => set it to zero
	diffs[0].difference = 0.0;

	//3. calculate the slopes
	TProcessedDifference& previousdiff = diffs[0];
	for (size_t i = 1; i < count; i++) {
		floattype kexpected = diffs[i].raw.expected - previousdiff.raw.expected;
		floattype kcalculated = diffs[i].raw.calculated - previousdiff.raw.calculated;
		floattype timedelta = diffs[i].raw.datetime - previousdiff.raw.datetime;

		{
			floattype invtimedelta = 1.0 / timedelta;
			kexpected *= invtimedelta;
			kcalculated *= invtimedelta;
		}

		/*

		Slopes only do not seem to provide any advantage over differences. Let's modify these into
		calculating glc level one hour prior left level.

		Actually, it does not seem to work better either. So, we leave because differences only would treat

		y1 = kx + q1
		y2 = kx + q2

		with q1 != q2

		*/
		kexpected = previousdiff.raw.expected - kexpected*OneHour;
		kcalculated = previousdiff.raw.expected - kcalculated*OneHour;



		floattype tmp = kexpected - kcalculated;

		if (mParameters.UseRelativeValues) {
			if (kexpected != 0.0)
				//tmp /= kexpected;	//cannot divide by zero, se could leave it as absolute one
				tmp = 0.0;	//but let us rather ignore it as we would compare apples with pears
		}
		if (mParameters.UseSquaredDifferences) tmp *= tmp;
		else tmp = fabs(tmp);

		diffs[i].difference = tmp;

		previousdiff = diffs[i];
	}
}

HRESULT IfaceCalling CCommonDiffMetric::Accumulate(TDifferencePoint *differences, size_t count, size_t alllevelscount) {
	mDifferences.reserve(mDifferences.size() + count);

	for (size_t i = 0; i < count; i++) {
		TProcessedDifference tmp;
		tmp.raw = differences[i];
		mDifferences.push_back(tmp);
	}

	mAllLevelsCount += alllevelscount;

	return S_OK;
}


HRESULT IfaceCalling CCommonDiffMetric::Reset() {
	mDifferences.clear();
	mAllLevelsCount = 0;
	return S_OK;
}


HRESULT IfaceCalling CCommonDiffMetric::Calculate(floattype *metric, size_t *levelsaccumulated, size_t levelsrequired) {
	size_t count = mDifferences.size();

	*levelsaccumulated = count;
	levelsrequired = std::max((decltype(levelsrequired))1, levelsrequired);

	if (/* (count<1) || */ (count < levelsrequired)) return S_FALSE;

	switch (mParameters.DifferenceProcessing) {
		case pdaNone:			break;
		case pdaDifferences:	ProcessDifferencesAsDifferences();	break;
		case pdaSlopes:			ProcessDifferencesAsSlopes();		break;
		case pdaISO2013:		ProcessDifferencesWithISO2013Tolerance(); break;
		default:				return E_INVALIDARG;
	}
	floattype localmetric = DoCalculateMetric();	//caching into the register
	//if (isnan(localmetric)) return S_FALSE;
	auto cl = fpclassify(localmetric);
	if ((cl != FP_NORMAL) && (cl != FP_ZERO)) return S_FALSE;

	if (mParameters.PreferMoreLevels != 0) localmetric /= (floattype)count;

	*metric = localmetric;


	return S_OK;
}