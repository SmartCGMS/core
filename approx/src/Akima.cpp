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

#include "Akima.h"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <assert.h>

#undef max

CAkima::CAkima(scgms::WSignal signal, scgms::IApprox_Parameters_Vector* configuration) : mSignal(signal) {
	Update();
}


bool CAkima::Update() {
	std::lock_guard<std::mutex> local_guard{ mUpdate_Guard };

	size_t update_count;
	if (mSignal.Get_Discrete_Bounds(nullptr, nullptr, &update_count) != S_OK)
		return false;

	if (update_count < MINIMUM_NUMBER_POINTS) return false;

	if (mInputTimes.size() != update_count) {
		mInputTimes.resize(update_count);
		mInputLevels.resize(update_count);
		mCoefficients.resize(update_count);
	}
	else // valCount == oldCount, no need to update
		return true;

	size_t filled;
	if (mSignal.Get_Discrete_Levels(mInputTimes.data(), mInputLevels.data(), update_count, &filled) != S_OK) {
		//error, we need to recalculate everything
		mInputTimes.clear();
		return false;
	}

	Compute_Coefficients();
	

	return true;
}

void CAkima::Compute_Coefficients() {
	const size_t count = mInputLevels.size();

	std::vector<double> coefsOfPolynFunc(4 * count);
	auto firstDerivatives = &coefsOfPolynFunc[count];

	coefsOfPolynFunc[0] = mInputLevels[0];
	coefsOfPolynFunc[1] = mInputLevels[1];

	double d1 = (mInputLevels[1] - mInputLevels[0]) / (mInputTimes[1] - mInputTimes[0]);
	double d2 = (mInputLevels[2] - mInputLevels[1]) / (mInputTimes[2] - mInputTimes[1]);

	double d3 = (mInputLevels[3] - mInputLevels[2]) / (mInputTimes[3] - mInputTimes[2]);
	double d4 = (mInputLevels[4] - mInputLevels[3]) / (mInputTimes[4] - mInputTimes[3]);


	double w1 = fabs(d1 - d2);
	double w2 = fabs(d2 - d3);
	double w3 = fabs(d3 - d4);

	double fd, fdPrev = 0.0;


	auto computeFd = [&](size_t i) {
		if (FP_ZERO == std::fpclassify(w3) && FP_ZERO == std::fpclassify(w1)) {
			double xv = mInputTimes[i]; // no need to optimize this,
			double xvP = mInputTimes[i + 1]; // expecting to be very rare case
			double xvM = mInputTimes[i - 1];
			return (((xvP - xv) * d2) + ((xv - xvM) * d3)) / (xvP - xvM);
		}
		else {
			return ((w3 * d2) + (w1 * d3)) / (w3 + w1);
		}
	};

	auto compute3th4thCoefs = [&](size_t i) {
		//copute 3. and 4. coeficients
		double w = mInputTimes[i]- mInputTimes[i - 1];
		double w_2 = w * w;

		double yv = mInputLevels[i - 1];
		double yvP = mInputLevels[i];

		//saving one division
		double divTmp = (yv - yvP) / w;
		coefsOfPolynFunc[2 * count + i - 1] = (-3 * divTmp - 2 * fdPrev - fd) / w;
		coefsOfPolynFunc[3 * count + i - 1] = (2 * divTmp + fdPrev + fd) / w_2;
	};


	for (size_t i = 2; i < count - 2 - 1; i++) {
		coefsOfPolynFunc[i] = mInputLevels[i];

		fd = computeFd(i);

		if (i != 2) { //is not first loop
			compute3th4thCoefs(i);
		}

		fdPrev = fd;
		firstDerivatives[i] = fd;

		d1 = d2;
		d2 = d3;
		d3 = d4;
		d4 = (mInputLevels[3 + i] - mInputLevels[2 + i]) / (mInputTimes[3 + i] - mInputTimes[2 + i]);

		w1 = w2;
		w2 = w3;
		w3 = fabs(d3 - d4);
	}

	size_t i = count - 3;
	//last iteration
	coefsOfPolynFunc[i] = mInputLevels[i];
	fd = computeFd(i);

	compute3th4thCoefs(i);

	fdPrev = fd;
	firstDerivatives[i] = fd;

	//compute last elements of 3. and 4. coefs
	i++;
	compute3th4thCoefs(i);

	coefsOfPolynFunc[count - 2] = mInputLevels[count - 2];
	coefsOfPolynFunc[count - 1] = mInputLevels[count - 1];

	firstDerivatives[0] = Differentiate_Three_Point_Scalar(0, 0, 1, 2);
	firstDerivatives[1] = Differentiate_Three_Point_Scalar(1, 0, 1, 2);
	firstDerivatives[count - 2] = Differentiate_Three_Point_Scalar(count - 2, count - 3, count - 2, count - 1);
	firstDerivatives[count - 1] = Differentiate_Three_Point_Scalar(count - 1, count - 3, count - 2, count - 1);

	mCoefficients = Interpolate_Hermite_Scalar(coefsOfPolynFunc);
}

double CAkima::Differentiate_Three_Point_Scalar(size_t indexOfDifferentiation, //0, 1, -2, -1
												size_t indexOfFirstSample, //0, 0, -3, -3
												size_t indexOfSecondsample, //1, 1, -2, -2
												size_t indexOfThirdSample) { //2, 2, -1, -1

	double x0 = mInputLevels[indexOfFirstSample];
	double x1 = mInputLevels[indexOfSecondsample];
	double x2 = mInputLevels[indexOfThirdSample];

	double t = mInputTimes[indexOfDifferentiation] - mInputTimes[indexOfFirstSample];
	double t1 = mInputTimes[indexOfSecondsample] - mInputTimes[indexOfFirstSample];
	double t2 = mInputTimes[indexOfThirdSample] - mInputTimes[indexOfFirstSample];

	double a = (x2 - x0 - (t2 / t1 * (x1 - x0))) / (t2 * t2 - t1 * t2);
	double b = (x1 - x0 - a * t1 * t1) / t1;

	return (2 * a * t) + b;
}


std::vector<double> CAkima::Interpolate_Hermite_Scalar(std::vector<double> coefsOfPolynFunc) {
	
	const size_t count = mInputLevels.size();
	auto firstDerivatives = &coefsOfPolynFunc[count];
	size_t numberOfDiffAndWeightElements = count - 1;
		//we should not get here with count less than 5

	size_t dimSize = count;
	for (size_t i = 0; i < numberOfDiffAndWeightElements; i++) {
		double w = mInputTimes[i + 1] - mInputTimes[i];
		double w2 = w * w;

		double yv = mInputLevels[i];
		double yvP = mInputLevels[i + 1];

		double fd = firstDerivatives[i];
		double fdP = firstDerivatives[i + 1];

		double divTmp = (yv - yvP) / w;

		coefsOfPolynFunc[2 * dimSize + i] = (-3 * divTmp - 2 * fd - fdP) / w;
		coefsOfPolynFunc[3 * dimSize + i] = (2 * divTmp + fd + fdP) / w2;
	}

	return coefsOfPolynFunc;
}



HRESULT IfaceCalling CAkima::GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) {

	assert((times != nullptr) && (levels != nullptr) && (count > 0));
	if ((times == nullptr) || (levels == nullptr)) return E_INVALIDARG;

	if (!Update() || mCoefficients.empty()) return E_FAIL;

	if (derivation_order > scgms::apxFirst_Order_Derivation) return E_INVALIDARG;

	const size_t measured_size_mul3 = mInputLevels.size() * 3;
	const size_t measured_size_mul2 = mInputLevels.size() * 2;
	const size_t measured_size = mInputLevels.size();
	//const double max_time = mInputTimes[mInputTimes.size() - 1];

	for (size_t i = 0; i< count; i++) {
		size_t knot_index = std::numeric_limits<size_t>::max();
		if (times[i] == mInputTimes[0]) knot_index = 0;
		else if (times[i] == mInputTimes[mInputTimes.size() - 1]) knot_index = mInputTimes.size() - 1;
		else if (!std::isnan(times[i])) {
			std::vector<double>::iterator knot_iter = std::upper_bound(mInputTimes.begin(), mInputTimes.end(), times[i]);
			if (knot_iter != mInputTimes.end()) knot_index = std::distance(mInputTimes.begin(), knot_iter) - 1;
		}



		if (knot_index != std::numeric_limits<size_t>::max()) {
			const double desired_time_knot_offset = times[i] - mInputTimes[knot_index];
			
			double res = 0.0;
			switch (derivation_order) {
				//Horner's evaluation method
				case scgms::apxNo_Derivation:
					res = mCoefficients[measured_size_mul3 + knot_index];
					res = desired_time_knot_offset * res + mCoefficients[measured_size_mul2 + knot_index];
					res = desired_time_knot_offset * res + mCoefficients[measured_size + knot_index];
					levels[i] = desired_time_knot_offset * res + mCoefficients[/*measured_size * 0*/ +knot_index];
					break;

				case scgms::apxFirst_Order_Derivation:
					res = 3.0 * mCoefficients[measured_size_mul3 + knot_index];
					res = desired_time_knot_offset * res + 2.0 * mCoefficients[measured_size_mul2 + knot_index];
					levels[i] = desired_time_knot_offset * res + mCoefficients[measured_size + knot_index];
					break;

				default: levels[i] = 0.0;
					break;
			}
		}
		else
			levels[i] = std::numeric_limits<double>::quiet_NaN();
	}

	return count > 0 ? S_OK : S_FALSE;
}