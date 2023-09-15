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

#include "physical_activity.h"
#include "../descriptor.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/utils/DebugHelper.h"

#include <cmath>

namespace {
	constexpr double MaxHeartRate = 200.0; // computational upper limit of heart rate
	//constexpr double EDAThreshold = 0.0;	--currently unused
}

CPhysical_Activity_Detection_Model::CPhysical_Activity_Detection_Model(scgms::WTime_Segment segment)
	: CCommon_Calculated_Signal(segment), mHeartRate(segment.Get_Signal(scgms::signal_Heartbeat)), mElectrodermalActivity(segment.Get_Signal(scgms::signal_Electrodermal_Activity)) {
	//
}

HRESULT IfaceCalling CPhysical_Activity_Detection_Model::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const
{
	const physical_activity_detection::TParameters &parameters = scgms::Convert_Parameters<physical_activity_detection::TParameters>(params, physical_activity_detection::default_parameters);

	const double historyTimeStep = scgms::One_Minute * 5.0;	// 5 minute step
	const size_t historyTimeCnt = 6;

	std::vector<double> htimes(historyTimeCnt);

	for (size_t i = 0; i < count; i++)
	{
		// generate history times
		for (size_t p = 0; p < historyTimeCnt; p++)
			htimes[historyTimeCnt - p - 1] = times[i] - static_cast<double>(p) * historyTimeStep;

		// select maximum from history times

		std::vector<double> sensor_readings(historyTimeCnt);
		double hrmax = std::numeric_limits<double>::quiet_NaN();
		double edamax = std::numeric_limits<double>::quiet_NaN();

		if (mHeartRate->Get_Continuous_Levels(nullptr, htimes.data(), sensor_readings.data(), historyTimeCnt, scgms::apxNo_Derivation) == S_OK) {
			for (size_t p = 0; p < historyTimeCnt; p++) {
				if (!std::isnan(sensor_readings[p]) && (std::isnan(hrmax) || sensor_readings[p] > hrmax)) {
					hrmax = sensor_readings[p];
				}
			}
		}

		if (mElectrodermalActivity && mElectrodermalActivity->Get_Continuous_Levels(nullptr, htimes.data(), sensor_readings.data(), historyTimeCnt, scgms::apxNo_Derivation) == S_OK) {
			for (size_t p = 0; p < historyTimeCnt; p++) {
				if (!std::isnan(sensor_readings[p]) && (std::isnan(edamax) || sensor_readings[p] > edamax)) {
					edamax = sensor_readings[p];
				}
			}
		}

		levels[i] = 0.0;

		// "detect" activity based on current max and resting heart rate ratio
		if (!std::isnan(hrmax) && hrmax > parameters.heartrate_resting) {
			levels[i] += ( (std::min(hrmax, MaxHeartRate) - parameters.heartrate_resting) / (MaxHeartRate - parameters.heartrate_resting) );
		}

		// if we have data from electrodermal activity sensor, we may want to "confirm" the activity index
		if (!std::isnan(edamax)) {

			if (edamax > parameters.eda_threshold)
			{
				const double thrmaxdiff = std::max(parameters.eda_max - parameters.eda_threshold + 1.0, 1.0);
				levels[i] *= 1.0 + (  ((std::min(edamax, parameters.eda_max) - parameters.eda_threshold) / thrmaxdiff) * 0.25  );
			}
			else
			{
				levels[i] *= 0.75;
			}
		}
	}

	return S_OK;
}

HRESULT IfaceCalling CPhysical_Activity_Detection_Model::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(physical_activity_detection::default_parameters);
	return parameters->set(params, params + physical_activity_detection::param_count);
}
