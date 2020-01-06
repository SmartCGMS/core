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

#include "cob.h"
#include "..\descriptor.h"

#include <numeric>

#include "../../../../common/rtl/rattime.h"
#include "../../../../common/rtl/SolverLib.h"

CCarbohydrates_On_Board::CCarbohydrates_On_Board(scgms::WTime_Segment segment)
	: CCommon_Calculated_Signal(segment), mSource_Signal(segment.Get_Signal(scgms::signal_Carb_Intake)) {
}

double CCarbohydrates_On_Board_Bilinear::Calculate_Signal(double bolusTime, double bolusValue, double nowTime, double peak, double dia) const
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
	//const double activityPeak = 2.0 / (dia / scgms::One_Minute);

	if (scaledTime < default_peak)
	{
		const double x1 = (scaledTime / 5.0) + 1.0;  // scaled minutes since bolus, pre-peak; divided by 5 to work with coefficients estimated based on 5 minute increments
		value = bolusValue * (1.0 + 0.001852*x1*(1.0 - x1));
	}
	else if (scaledTime < default_end)
	{
		const double x2 = ((scaledTime - default_peak) / 5.0);  // scaled minutes past peak; divided by 5 to work with coefficients estimated based on 5 minute increments
		value = bolusValue * (0.555560 + x2 * (0.001323*x2 - 0.054233));
	}
	// if the treatment does no longer have effect, return default-initialized IOB struct (zero values)

	return value;
}

double CCarbohydrates_On_Board::Calculate_Total_COB(double nowTime, double peak, double dia) const
{
	double totalCob = 0;
	size_t cnt, filled;
	std::vector<double> carbTimes, carbLevels;

	// get carbs levels and times, add it to total COB contrib
	if (mSource_Signal->Get_Discrete_Bounds(nullptr, nullptr, &cnt) == S_OK && cnt != 0)
	{
		carbTimes.resize(cnt);
		carbLevels.resize(cnt);
		
		if (mSource_Signal->Get_Discrete_Levels(carbTimes.data(), carbLevels.data(), cnt, &filled) == S_OK)
		{
			if (cnt != filled)
			{
				carbTimes.resize(filled);
				carbLevels.resize(filled);
				cnt = filled;
			}

			for (size_t i = 0; i < cnt; i++)
			{
				if (carbTimes[i] > nowTime)
					continue;

				double tVal = Calculate_Signal(carbTimes[i], carbLevels[i], nowTime, peak, dia);
				totalCob += tVal;
			}
		}
	}

	return totalCob;
}

HRESULT CCarbohydrates_On_Board::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const
{
	iob::TParameters &parameters = scgms::Convert_Parameters<iob::TParameters>(params, iob::default_parameters);

	for (size_t i = 0; i < count; i++)
	{
		double totalCob = Calculate_Total_COB(times[i], parameters.peak, parameters.dia);

		levels[i] = totalCob;
	}

	return S_OK;
}

HRESULT CCarbohydrates_On_Board::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const
{
	double *params = const_cast<double*>(cob::default_parameters);
	return parameters->set(params, params + cob::param_count);
}
