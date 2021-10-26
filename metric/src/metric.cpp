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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "metric.h"

#include <algorithm>
#include <math.h>
#include <limits>

#undef max
#undef min


double CAbsDiffAvgMetric::Do_Calculate_Metric() {
	/*
	//Kahan sum: http://msdn.microsoft.com/en-us/library/aa289157%28v=vs.71%29.aspx

	double C = 0.0;
	double accumulator 0.0;


	for (size_t i=0; i<count; i++) {
		double Y = diffs[i].difference - C;
		double T = accumulator + Y;
		C = T - accumulator - Y;
		accumulator = T;
	}
	*/

	double accumulator = 0.0;
	
	for (const auto &diff : mDifferences) {
		accumulator += diff.difference;
	}

	return accumulator / static_cast<double>(mDifferences.size());
}


double CExpWeightedDiffAvgPolar_Metric::Do_Calculate_Metric() {
	double accumulator = 0.0;

	for (const auto &diff : mDifferences) {

		double accdiff = diff.difference;
		if (diff.raw.calculated + mParameters.threshold < diff.raw.expected)
			accdiff *= 1.0 + diff.difference;

		accumulator += accdiff;
	}

	return accumulator / static_cast<double>(mDifferences.size());
}

double CAbsDiffMaxMetric::Do_Calculate_Metric() {
	
	double maximum = -std::numeric_limits<double>::max();
	for (const auto &diff : mDifferences) {
		maximum = std::max(diff.difference, maximum);
	}

	return maximum;
}

CAbsDiffPercentilMetric::CAbsDiffPercentilMetric(scgms::TMetric_Parameters& params) : CCommon_Metric(params) {
	mInvThreshold = 0.01 * params.threshold;
};

double CAbsDiffPercentilMetric::Do_Calculate_Metric() {
	size_t count = mDifferences.size();
	size_t offset = static_cast<size_t>(round((static_cast<double>(count))*mInvThreshold));
	offset = std::min(offset, count - 1); //handles negative value as well

	std::partial_sort(mDifferences.begin(),
		mDifferences.begin()+offset, //middle
		mDifferences.end(),
		[](const TProcessed_Difference &a, const TProcessed_Difference &b) -> bool {
			return a.difference < b.difference;
	}
	);

	if (offset > 0) offset--;	//elements are sorted up to middle, i.e, middle is not sorted

	return mDifferences[offset].difference;	
}



double CAbsDiffThresholdMetric::Do_Calculate_Metric() {
	sort(mDifferences.begin(), mDifferences.end(), 
		[](const TProcessed_Difference &a, const TProcessed_Difference &b) -> bool {
			return a.difference < b.difference; }
	);
	//we need to determine how many levels were calculated until the desired threshold 
	//out of all values that could be calculated
	auto thebegin = mDifferences.begin();

	const TProcessed_Difference thresh = { { 0.0, 0.0, 0.0 }, mParameters.threshold*0.01 };

	auto iter = std::upper_bound(thebegin, mDifferences.end(), thresh,
		[](const TProcessed_Difference &a, const TProcessed_Difference &b) -> bool {
			return a.difference < b.difference; }	//a always equal to thresh
		);

	//return 1.0 / std::distance(thebegin, iter);
	const auto thresholdcount = std::distance(thebegin, iter);
	return static_cast<double>(mDifferences.size() - thresholdcount);
}


CLeal2010Metric::CLeal2010Metric(scgms::TMetric_Parameters& params) : CCommon_Metric({params.metric_id, false, true, params.prefer_more_levels, params.threshold}) {
	//mParameters.use_relative_error = false;
	//mParameters.use_squared_differences = true;
}

double CLeal2010Metric::Do_Calculate_Metric() {

	double diffsqsum = 0.0;
	double avgedsqsum = 0.0;

	double expectedsum = 0.0;
	for (const auto &diff : mDifferences) {
		expectedsum += diff.raw.expected;
	}

	const double avg = expectedsum / static_cast<double>(mDifferences.size());

	for (const auto &diff : mDifferences) {
		double tmp = diff.difference;
		diffsqsum += tmp*tmp;

		tmp = diff.raw.expected - avg;

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

CAICMetric::CAICMetric(scgms::TMetric_Parameters& params) : CAbsDiffAvgMetric({ params.metric_id, false, true, params.prefer_more_levels, params.threshold }){
	//mParameters.UseRelativeValues = false;
	//mParameters.UseSquaredDifferences = true;
};


double CAICMetric::Do_Calculate_Metric() {
	double n = static_cast<double>(mDifferences.size());
	return n*log(CAbsDiffAvgMetric::Do_Calculate_Metric()); 
}

CRMSE_Metric::CRMSE_Metric(scgms::TMetric_Parameters& params) : CAbsDiffAvgMetric({ params.metric_id, params.use_relative_error, true, params.prefer_more_levels, params.threshold }) { };
//mParameters.UseSquaredDifferences = true;

double CRMSE_Metric::Do_Calculate_Metric() {
	return sqrt(CAbsDiffAvgMetric::Do_Calculate_Metric());
}



double CVariance_Metric::Do_Calculate_Metric() {

	//threshold holds margins to cut off, so we have to sort first
	sort(mDifferences.begin(), mDifferences.end(), 
		[](const TProcessed_Difference &a, const TProcessed_Difference &b) -> bool {
		return a.difference < b.difference; }
	);

	size_t lowbound, highbound;
	{
		double n = static_cast<double>(mDifferences.size());
		double margin = n*0.01*mParameters.threshold;
		lowbound = (size_t)floor(margin);
		highbound = (size_t)ceil(n - margin);

	}

	if (lowbound > highbound)
		return std::numeric_limits<double>::quiet_NaN();

	double sum = 0.0;	
	for (auto i = lowbound; i != highbound; i++) {
		sum += mDifferences[i].difference;
	}

	const double casted_size = static_cast<double>(mDifferences.size());
	double invn = casted_size;

	//first, try Unbiased estimation of standard deviation
	if (invn > 1.5) 			//since invn is alway a full number (no fraction due to being a number of items in the vector),
								//we just need to know whether it is greater than 1
		invn = casted_size - 1.5 + 1 / (8 * (casted_size - 1));	//precise estimation, better than substracting 1.5	
		else if (invn > 1.0) invn -= 1.0;	//if not, try to fall back to Bessel's Correction at least
	invn = 1.0 / invn;

	mLast_Calculated_Avg = sum*invn;

	sum = 0.0;
	for (auto i = lowbound; i != highbound; i++) {
		double tmp = mDifferences[i].difference - mLast_Calculated_Avg;
		sum += tmp*tmp;
	}

	return sum*invn;
}

double CStdDevMetric::Do_Calculate_Metric() {
	return sqrt(CVariance_Metric::Do_Calculate_Metric());
}


double CAvgPlusBesselStdDevMetric::Do_Calculate_Metric() {
	const double variance = CVariance_Metric::Do_Calculate_Metric();
			//also calculates mLastCalculatedAvg
	return mLast_Calculated_Avg + sqrt(variance);
}



double CAvg_Pow_StdDev_Metric::Do_Calculate_Metric() {
	const double variance = CVariance_Metric::Do_Calculate_Metric();

	return log(mLast_Calculated_Avg) + log(sqrt(variance));

	//also calculates mLastCalculatedAvg

		//the real metric is supposed to be
		//(1.0+avg)^(1.0+std_dev_estimate)		
		//both 1.0+ are to avoid adverse results with both avg and sd less than 1.0 so the best metric is 1.0 => -1.0 to have zero as the best fit like the other metrics
	//return pow(1.0+mLast_Calculated_Avg, 1.0 + sqrt(variance))-1.0;	//we do sqrt to minimize the power extent


	/*
	//statistical efficiency
	if (mLast_Calculated_Avg >= 0.0) {
		return variance / mLast_Calculated_Avg*mLast_Calculated_Avg;	
	}
	else
		return 0.0;	//zero average means no error
	*/

	//coefficient of determination - comes almost close to avg+sd
	double sum_of_residuals = 0.0;
	double sum_observed = 0.0;
	for (const auto& elem : mDifferences) {
		sum_of_residuals += elem.difference * elem.difference;
		sum_observed += elem.raw.expected;
	}


	double corrected_count = static_cast<double>(mDifferences.size());	//this would be the original, but we make corrected estimation like sd-estimation
	if (corrected_count > 1.5) 
		corrected_count = corrected_count - 1.5 + 1 / (8 * (corrected_count - 1));	//precise estimation, better than substracting 1.5	
		else if (corrected_count > 1.0) corrected_count -= 1.0;	//if not, try to fall back to Bessel's Correction at least

	const double avg_observed = sum_observed / corrected_count;
	double sum_of_total = 0.0;
	for (const auto& elem : mDifferences) {
		const double diff = elem.raw.expected - avg_observed;
		sum_of_total += diff * diff;
	}

	return sum_of_residuals / sum_of_total;

	

	//harmonic mean
	double n = 0.0;
	double sum = 0.0;
	for (const auto& elem : mDifferences) {
		if (elem.difference != 0.0) {
			n += 1.0;
			sum += 1.0 / std::fabs(elem.difference);
		}
	}
	return n / sum;

	/*
	//coefficient of variation
	if (mLast_Calculated_Avg >= 0.0) {
		return sqrt(variance) / mLast_Calculated_Avg;	//coefficient of variation		
	}
	else
		return 0.0;	//zero average means no error
	

	//quartile coefficient of dispersion
	std::sort(mDifferences.begin(),
		mDifferences.end(),
		[](const TProcessed_Difference &a, const TProcessed_Difference &b) -> bool {
		return a.difference < b.difference;
	}
	);
	const size_t q1_idx = static_cast<size_t>(round(0.25*static_cast<double>(mDifferences.size())));
	const size_t q3_idx = static_cast<size_t>(round(0.75*static_cast<double>(mDifferences.size())));
	const double q1 = mDifferences[q1_idx].difference;
	const double q3 = mDifferences[q3_idx].difference;
	//const double interquartile_range = 0.5*(q3 - q1);
	//const double midhinge = 0.5*(q1 + q3);
	return (q3 - q1) / (q1 + q3); //interquartile_range / midhinge;


	*/

	

//https://en.wikipedia.org/wiki/Coefficient_of_variation
//https://en.wikipedia.org/wiki/Coefficient_of_determination
}




double CCrossWalkMetric::Do_Calculate_Metric() {
	/*
		We will calculate path from first measured to second calculated, then to third measured, until we reach the last difference.
		Then, we will do the same, but starting with calculated level. As a result we get a length of pass that we would
		have to traverse and which will equal to double length of the measured curve, if both curves would be identical.
	*/

	//1. we have to sort differences accordingly to the time
	sort(mDifferences.begin(), mDifferences.end(),
		[](const TProcessed_Difference &a, const TProcessed_Difference &b)->bool {
		return a.raw.datetime < b.raw.datetime; }
	);

	//2. for the first differences, the path-length cannot be calculated at all => set it to zero
	mDifferences[0].difference = 0.0; //this method will not be called with count<1

	double path_length = 0.0;
	double measured_path_length = 0.0;	//ideal path through the measured points only

	

	for (size_t i = 1; i < mDifferences.size(); i++) {
		const TProcessed_Difference& previous_diff = mDifferences[i-1];
		const TProcessed_Difference& current_diff = mDifferences[i];

		double timedelta = current_diff.raw.datetime - previous_diff.raw.datetime;
		timedelta *= timedelta;

		double mesA = previous_diff.raw.expected;
		double calcA = previous_diff.raw.calculated;

		double mesB = current_diff.raw.expected;
		double calcB = current_diff.raw.calculated;


		if (mParameters.use_relative_error) {
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


		bool mestocalccross = mCross_Measured_With_Calculated_Only || ((mesA>calcA) & (mesB > calcB)) || ((mesA < calcA) & (mesB < calcB));

		
		if (mestocalccross) {

			//Measured-to-calculated and calculated-to-measured
			double height = mesA - calcB;	//do not forget fabs when experimenting with odd-powers
			height *= height;
			if (mCrosswalk4) height *= height;

			path_length += sqrt(timedelta + height);

			height = calcA - mesB;				//do not forget fabs when experimenting with odd-powers
			height *= height;
			if (mCrosswalk4) height *= height;
			path_length += sqrt(timedelta + height);

		}
		else {
			//Measured-to-measured and calculated-to-calculated
			double height = mesA - mesB;
			height *= height;
			if (mCrosswalk4) height *= height;
			path_length += sqrt(timedelta + height);


			height = calcA - calcB;
			height *= height;
			if (mCrosswalk4) height *= height;
			path_length += sqrt(timedelta + height);
		}

		if (mCompare_To_Measured_Path) {
			double mes_height = mesA - mesB;
			mes_height *= mes_height;
			if (mCrosswalk4) mes_height *= mes_height;
			measured_path_length += sqrt(timedelta + mes_height);
		}
	}
	
	if (mCompare_To_Measured_Path)	path_length = abs(path_length - measured_path_length);

	return path_length / static_cast<double>(mDifferences.size());
}




double CIntegralCDFMetric::Do_Calculate_Metric() {

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
		[](const TProcessed_Difference &a, const TProcessed_Difference &b)->bool {
		return a.difference < b.difference; }
	);

	size_t lowbound, highbound;
	{
		double n = static_cast<double>(mDifferences.size());
		double margin = n*0.01*mParameters.threshold;
		lowbound = (size_t)floor(margin);
		highbound = (size_t)ceil(n - margin);

	}

	if (lowbound >= highbound)
		return std::numeric_limits<double>::quiet_NaN();


	double area = 0.0;
	double step = 1.0 / static_cast<double>(mDifferences.size());

	auto diffs = &mDifferences[0];
	auto previous = lowbound;
	lowbound++;
	for (auto i = lowbound; i != highbound; i++) {

		area += step*(std::min(diffs[previous].difference, diffs[i].difference) + fabs(diffs[previous].difference - diffs[i].difference)*0.5);
	}

	return area*step;
}
