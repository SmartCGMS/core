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

#include "iob.h"
#include "../descriptor.h"

#include <numeric>
#include <cmath>

#include <scgms/rtl/rattime.h>
#include <scgms/rtl/SolverLib.h>

CInsulin_Absorption::CInsulin_Absorption(scgms::WTime_Segment segment, NInsulin_Calc_Mode mode) : CCommon_Calculated_Signal(segment), mDelivered_Insulin(segment.Get_Signal(scgms::signal_Delivered_Insulin_Total)),
	mMode(mode) {
	if (!refcnt::Shared_Valid_All(mDelivered_Insulin)) throw std::exception{};
}

double CInsulin_Absorption_Bilinear::Calculate_Signal(double bolusTime, double bolusValue, double nowTime, double peak, double dia) const
{
	double value = 0.0;

	// NOTE: oref0 math assumes inputs in minutes (so the coefficients would work)

	const double default_dia = 180.0;		// assumed duration of insulin activity, in minutes
	const double default_peak = 75.0;		// assumed peak insulin activity, in minutes
	const double default_end = 180.0;		// assumed end of insulin activity, in minutes

	// Scale minsAgo by the ratio of the default dia / the user's dia 
	// so the calculations for activityContrib and iobContrib work for 
	// other dia values (while using the constants specified above)
	const double scaledTime = (default_dia / (dia/scgms::One_Minute)) * ((nowTime - bolusTime) / scgms::One_Minute);

	// Calc percent of insulin activity at peak, and slopes up to and down from peak
	// Based on area of triangle, because area under the insulin action "curve" must sum to 1
	// (length * height) / 2 = area of triangle (1), therefore height (activityPeak) = 2 / length (which in this case is dia, in minutes)
	// activityPeak scales based on user's dia even though peak and end remain fixed
	const double activityPeak = 2.0 / (dia / scgms::One_Minute);

	if (scaledTime < default_peak)
	{
		if (mMode == NInsulin_Calc_Mode::Activity) {
			const double slopeUp = activityPeak / default_peak;
			value = bolusValue * (slopeUp * scaledTime);
		}
		else if (mMode == NInsulin_Calc_Mode::IOB) {
			const double x1 = (scaledTime / 5.0) + 1.0;  // scaled minutes since bolus, pre-peak; divided by 5 to work with coefficients estimated based on 5 minute increments
			value = bolusValue * (1.0 + 0.001852*x1*(1.0 - x1));
		}
	}
	else if (scaledTime < default_end)
	{
		if (mMode == NInsulin_Calc_Mode::Activity) {
			const double slopeDown = -1.0 * (activityPeak / (default_end - default_peak));
			const double minsPastPeak = scaledTime - default_peak;
			value = bolusValue * (activityPeak + (slopeDown * minsPastPeak));
		}
		else if (mMode == NInsulin_Calc_Mode::IOB) {
			const double x2 = ((scaledTime - default_peak) / 5.0);  // scaled minutes past peak; divided by 5 to work with coefficients estimated based on 5 minute increments
			value = bolusValue * (0.555560 + x2 * (0.001323*x2 - 0.054233));
		}
	}
	// if the treatment does no longer have effect, return default-initialized IOB struct (zero values)

	return value;
}

double CInsulin_Absorption_Exponential::Calculate_Signal(double bolusTime, double bolusValue, double nowTime, double peak, double dia) const
{
	double value = 0.0;

	// NOTE: oref0 math assumes inputs in minutes (so the coefficients would work)

	const double end = dia / scgms::One_Minute;  // end of insulin activity, in minutes
	const double minsAgo = (nowTime - bolusTime) / scgms::One_Minute;
	const double scaledPeak = peak / scgms::One_Minute;

	if (minsAgo < end) {

		// Formula source: https://github.com/LoopKit/Loop/issues/388#issuecomment-317938473
		// Mapping of original source variable names to those used here:
		//   td = end
		//   tp = peak
		//   t  = minsAgo
		const double tau = scaledPeak * (1.0 - scaledPeak / end) / (1.0 - 2.0 * scaledPeak / end);  // time constant of exponential decay
		const double a = 2.0 * tau / end;                                     // rise time factor
		const double S = 1.0 / (1.0 - a + (1.0 + a) * std::exp(-end / tau));      // auxiliary scale factor

		if (mMode == NInsulin_Calc_Mode::Activity)
			value = bolusValue * (S / std::pow(tau, 2)) * minsAgo * (1.0 - minsAgo / end) * std::exp(-minsAgo / tau);
		else if (mMode == NInsulin_Calc_Mode::IOB)
			value = bolusValue * (1.0 - S * (1.0 - a) * ((std::pow(minsAgo, 2) / (tau * end * (1.0 - a)) - minsAgo / tau - 1.0) * std::exp(-minsAgo / tau) + 1.0));
	}

	return value;
}

IOB_Combined CInsulin_Absorption::Calculate_Total_IOB(double nowTime, double peak, double dia) const
{
	IOB_Combined totalIob = {0, 0};
	size_t cnt, filled;
	std::vector<double> insTimes, insLevels;

	// get basal insulin levels and times, add it to total IOB contrib
	if (mDelivered_Insulin->Get_Discrete_Bounds(nullptr, nullptr, &cnt) == S_OK && cnt != 0)
	{
		insTimes.resize(cnt);
		insLevels.resize(cnt);

		if (mDelivered_Insulin->Get_Discrete_Levels(insTimes.data(), insLevels.data(), cnt, &filled) == S_OK)
		{
			if (cnt != filled)
			{
				insTimes.resize(filled);
				insLevels.resize(filled);
				cnt = filled;
			}

			for (size_t i = 0; i < cnt; i++)
			{
				if (insTimes[i] > nowTime)
					continue;

				double tVal = Calculate_Signal(insTimes[i], insLevels[i], nowTime, peak, dia);
				totalIob.basal += tVal;
			}
		}
	}

	return totalIob;
}

HRESULT CInsulin_Absorption::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const
{
	const iob::TParameters &parameters = scgms::Convert_Parameters<iob::TParameters>(params, iob::default_parameters);

	for (size_t i = 0; i < count; i++)
	{
		IOB_Combined totalIob = Calculate_Total_IOB(times[i], parameters.peak, parameters.dia);

		levels[i] = totalIob.basal + totalIob.bolus;
	}

	return S_OK;
}

HRESULT CInsulin_Absorption::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const
{
	double *params = const_cast<double*>(iob::default_parameters);
	return parameters->set(params, params + iob::param_count);
}
