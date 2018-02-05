/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#include "metric.h"

#include "../../../common/rtl/manufactory.h"

#include <algorithm>
#include <math.h>
#include <limits>

#undef max
#undef min


floattype CAbsDiffAvgMetric::DoCalculateMetric() {
	/*
	//Kahan sum: http://msdn.microsoft.com/en-us/library/aa289157%28v=vs.71%29.aspx

	floattype C = 0.0;
	floattype accumulator 0.0;


	for (size_t i=0; i<count; i++) {
		floattype Y = diffs[i].difference - C;
		floattype T = accumulator + Y;
		C = T - accumulator - Y;
		accumulator = T;		
	}
	*/

	floattype accumulator = 0.0;
	size_t count = mDifferences.size();
	TProcessedDifference *diffs = &mDifferences[0];

	for (size_t i = 0; i < count; i++) {
		accumulator += diffs[i].difference;
	}

	return accumulator / (floattype)count;	
}


floattype CAbsDiffMaxMetric::DoCalculateMetric() {
	size_t count = mDifferences.size();
	TProcessedDifference *diffs = &mDifferences[0];
	
	floattype maximum = diffs[0].difference;
	for (size_t i = 1; i < count; i++) {
		maximum = std::max(diffs[i].difference, maximum);
	}

	return maximum;
}

CAbsDiffPercentilMetric::CAbsDiffPercentilMetric(TMetricParameters params) : CCommonDiffMetric(params) {
	mInvThreshold = 0.01 * params.Threshold;	
};

floattype CAbsDiffPercentilMetric::DoCalculateMetric() {
	size_t count = mDifferences.size();
	size_t offset = (size_t)round(((floattype)(count))*mInvThreshold);
	offset = std::min(offset, count - 1);//handles negative value as well

	std::partial_sort(mDifferences.begin(),
		mDifferences.begin()+offset, //middle
		mDifferences.end(),
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.difference < b.difference;
	}
	);

	if (offset>0) offset--;	//elements are sorted up to middle, i.e, middle is not sorted

	return mDifferences[offset].difference;	
}



floattype CAbsDiffThresholdMetric::DoCalculateMetric() {
	sort(mDifferences.begin(), mDifferences.end(), 
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.difference < b.difference; }
	);
	//we need to determine how many levels were calculated until the desired threshold 
	//out of all values that could be calculated
	auto thebegin = mDifferences.begin();

	const TProcessedDifference thresh = { { 0.0, 0.0, 0.0 }, mParameters.Threshold*0.01 };

	auto iter = std::upper_bound(thebegin, mDifferences.end(), thresh,
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.difference < b.difference; }	//a always equal to thresh
		);

	//return 1.0 / std::distance(thebegin, iter);
	size_t thresholdcount = std::distance(thebegin, iter);
	
	return (floattype) (mAllLevelsCount - thresholdcount);
}


CLeal2010Metric::CLeal2010Metric(TMetricParameters params) : CCommonDiffMetric(params) {
	mParameters.UseRelativeValues = false;
	mParameters.UseSquaredDifferences = true;
	mParameters.DifferenceProcessing = pdaDifferences;
}

floattype CLeal2010Metric::DoCalculateMetric() {
	
	size_t cnt = mDifferences.size();
	floattype diffsqsum = 0.0;
	floattype avgedsqsum = 0.0;

	floattype expectedsum = 0.0;
	for (size_t i = 0; i < cnt; i++) {
		expectedsum += mDifferences[i].raw.expected;
	}

	floattype avg = expectedsum / (floattype)cnt;

	for (size_t i = 0; i < cnt; i++) {
		floattype tmp = mDifferences[i].difference;		
		diffsqsum += tmp*tmp;

		tmp = mDifferences[i].raw.expected - avg;		

		avgedsqsum += tmp*tmp;
	}


	//return (1.0 - sqrt((diffsqsum / avgedsqsum)));
	/*
		This is the original metric where greater number is better.
		However, we need the opposite. We could either calculate
		an inverse value of the original metric, or change its sign.
		Changing the sign is better as we can get rid of the 1-constant
		and of the sqrt.
	*/

	return diffsqsum / avgedsqsum;
}

CAICMetric::CAICMetric(TMetricParameters params) : CAbsDiffAvgMetric(params){
	mParameters.UseRelativeValues = false;
	mParameters.UseSquaredDifferences = true;	
};


floattype CAICMetric::DoCalculateMetric() {
	floattype n = (floattype) mDifferences.size();
	return n*log(CAbsDiffAvgMetric::DoCalculateMetric()); 
}

floattype CStdDevMetric::DoCalculateMetric() {

	//threshold holds margins to cut off, so we have to sort first
	sort(mDifferences.begin(), mDifferences.end(), 
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.difference < b.difference; }
	);

	size_t lowbound, highbound;
	{
		floattype n = (floattype)mDifferences.size();
		floattype margin = n*0.01*mParameters.Threshold;
		lowbound = (size_t)floor(margin);
		highbound = (size_t)ceil(n - margin);

	}

	if (lowbound >= highbound)
		return std::numeric_limits<floattype>::quiet_NaN();


	floattype sum = 0.0;
	auto diffs = &mDifferences[0];
	for (auto i = lowbound; i != highbound; i++) {
		sum += diffs[i].difference;
	}


	floattype invn = (floattype)mDifferences.size();
	if (mDoBesselCorrection) {
		//first, try Unbiased estimation of standard deviation
		if (invn > 1.5) invn -= 1.5; 
		else if (invn > 1.0) invn -= 1.0;	//if not, try to fall back to Bessel's Correction at least
	}
	invn = 1.0 / invn;

	mLastCalculatedAvg = sum*invn;

	sum = 0.0;
	for (auto i = lowbound; i != highbound; i++) {
		floattype tmp = diffs[i].difference - mLastCalculatedAvg;
		sum += tmp*tmp;
	}

	return sum*invn;	//We should calculate squared root by definition, but it is useless overhead for us
						//so that we in-fact calculate variance
}

floattype CAvgPlusBesselStdDevMetric::DoCalculateMetric() {
	floattype variance = CStdDevMetric::DoCalculateMetric();
			//also calculates mLastCalculatedAvg
	return mLastCalculatedAvg + sqrt(variance);
}

CCrossWalkMetric::CCrossWalkMetric(TMetricParameters params) : CCommonDiffMetric(params) { 
	if (mParameters.DifferenceProcessing == pdaSlopes)	//take out invalid option
		mParameters.DifferenceProcessing = pdaDifferences;	
}

floattype CCrossWalkMetric::DoCalculateMetric() {
	/*
		We will calculate path from first measured to second calculated, then to third measured, until we reach the last difference.
		Then, we will do the same, but starting with calculated level. As a result we get a length of pass that we would
		have to traverse and which will equal to double length of the measured curve, if both curves would be identical.
		*/

	size_t count = mDifferences.size();
	TProcessedDifference *diffs = &mDifferences[0];	//this method will not be called with count<1

	//1. we have to sort differences accordingly to the time
	sort(mDifferences.begin(), mDifferences.end(),
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.raw.datetime < b.raw.datetime; }
	);

	//2. for the first differences, the path-length cannot be calculated at all => set it to zero
	diffs[0].difference = 0.0;

	floattype pathlength = 0.0;
	floattype measured_path_length = 0.0;	//ideal path through the measured points only

	TProcessedDifference& previousdiff = diffs[0];

	for (size_t i = 1; i < count; i++) {
		floattype timedelta = diffs[i].raw.datetime - previousdiff.raw.datetime;
		timedelta *= timedelta;

		floattype mesA = previousdiff.raw.expected;
		floattype calcA = previousdiff.raw.calculated;

		floattype mesB = diffs[i].raw.expected;
		floattype calcB = diffs[i].raw.calculated;


		if (mParameters.UseRelativeValues) {
			if (mCalculate_Real_Relative_Difference) {
				calcA = fabs(mesA - calcA) / mesA;
				mesA = 0.0;

				calcB = fabs(calcB - mesB) / mesB;
				mesB = 0.0;
			}
			else {
				calcA /= mesA;
				mesA = 1.0;

				//move mes/calcA are calculated twice - in the next iteration, they are calculated to current mes/calcB

				calcB /= mesB;
				mesB = 1.0;
			}
		}		


		bool mestocalccross = mCrossMeasuredWithCalculatedOnly || ((mesA>calcA) & (mesB > calcB)) || ((mesA < calcA) & (mesB < calcB));

		
		if (mestocalccross) {

			//Measured-to-calculated and calculated-to-measured
			floattype height = mesA - calcB;	//do not forget fabs when experimenting with odd-powers
			height *= height;
			if (mCrosswalk4) height *= height;

			pathlength += sqrt(timedelta + height);

			height = calcA - mesB;				//do not forget fabs when experimenting with odd-powers
			height *= height;
			if (mCrosswalk4) height *= height;
			pathlength += sqrt(timedelta + height);

		}
		else {
			//Measured-to-measured and calculated-to-calculated
			floattype height = mesA - mesB;		
			height *= height;
			if (mCrosswalk4) height *= height;
			pathlength += sqrt(timedelta + height);


			height = calcA - calcB;
			height *= height;
			if (mCrosswalk4) height *= height;
			pathlength += sqrt(timedelta + height);
		}

		if (mCompare_To_Measured_Path) {
			floattype mes_height = mesA - mesB;
			mes_height *= mes_height;
			if (mCrosswalk4) mes_height *= mes_height;
			measured_path_length += sqrt(timedelta + mes_height);
		}

		previousdiff = diffs[i];
	}
	
	if (mCompare_To_Measured_Path)	pathlength = abs(pathlength - measured_path_length);

	return pathlength / (floattype) count;
}



floattype CPath_Difference::DoCalculateMetric() {
	//1. we have to sort differences accordingly to the time
	sort(mDifferences.begin(), mDifferences.end(),
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.raw.datetime < b.raw.datetime; }
	);

	//2. and calculate path lengths

	floattype calc_path = 0.0;// fabs(mDifferences[0].raw.expected - mDifferences[0].raw.calculated);
	floattype mes_path = 0.0;

	for (size_t i = 1; i < mDifferences.size(); i++) {

		const auto &prev = mDifferences[i-1].raw;
		const auto &curr = mDifferences[i].raw;

		floattype prev_mes = prev.expected;
		floattype prev_calc = prev.calculated;

		floattype curr_mes = curr.expected;
		floattype curr_calc = curr.calculated;


		if (mParameters.UseRelativeValues) {
			/*prev_calc = fabs(prev_mes-prev_calc)/prev_mes;
			prev_mes = 0.0;

			curr_calc = fabs(curr_calc - curr_mes)/curr_mes;
			curr_mes = 0.0;
			*/

			prev_calc /= prev_mes;
			prev_mes = 1.0;

			curr_calc /= curr_mes;
			curr_mes = 1.0;
		}

		//actually, we calculate a saw - respective only the upper par of the length(hypotenuses)
		//
		//
		//         ... c
		//     ....
		// m...        m............c
		//                          m
		//
		//


		const floattype time_delta = curr.datetime - prev.datetime;
		const floattype time_delta_sq = time_delta*time_delta;
			
		const floattype calc_diff =  curr_calc - prev_mes;
		const floattype mes_diff = curr_mes - prev_mes;

		calc_path += sqrt(time_delta_sq + calc_diff*calc_diff);// +fabs(curr.expected - curr.expected);
		mes_path += sqrt(time_delta_sq + mes_diff*mes_diff);
	}

	return fabs(mes_path - calc_path);
}

floattype CIntegralCDFMetric::DoCalculateMetric() {

	/*
		Note that empirical cumulative distribution function will have relative error
		on the x-axis, while having the cumulative probability, i.e. relative error percentile 
		when sorted, on the y-axis. Then, we have to minimize the left upper area of the rectangle
		that is divided by the cdf curve. I.e., best solution maximizes the right bottom area.

		Therefore, we switch these axis so that we minimize the right bottom area as less metric
		value means a better fit.

	 */

	//threshold holds margins to cut off, so we have to sort first
	sort(mDifferences.begin(), mDifferences.end(),
		[](const TProcessedDifference &a, const TProcessedDifference &b)->bool {
		return a.difference < b.difference; }
	);

	size_t lowbound, highbound;
	{
		floattype n = (floattype)mDifferences.size();
		floattype margin = n*0.01*mParameters.Threshold;
		lowbound = (size_t)floor(margin);
		highbound = (size_t)ceil(n - margin);

	}

	if (lowbound >= highbound)
		return std::numeric_limits<floattype>::quiet_NaN();


	floattype area = 0.0;
	floattype step = 1.0 / (floattype)mDifferences.size();

	auto diffs = &mDifferences[0];
	auto previous = lowbound;
	lowbound++;
	for (auto i = lowbound; i != highbound; i++) {

		area += step*(std::min(diffs[previous].difference, diffs[i].difference) + fabs(diffs[previous].difference - diffs[i].difference)*0.5);
	}

	

	

	return area*step;	
}
