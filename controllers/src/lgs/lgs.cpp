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

#include "lgs.h"
#include "../descriptor.h"
#include <scgms/rtl/SolverLib.h>

#include <cmath>

CConstant_Basal_LGS_Insulin_Rate_Model::CConstant_Basal_LGS_Insulin_Rate_Model(scgms::WTime_Segment segment)
	: CCommon_Calculated_Signal(segment), mIG(segment.Get_Signal(scgms::signal_IG)) {
	//
}

HRESULT IfaceCalling CConstant_Basal_LGS_Insulin_Rate_Model::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	const lgs_basal_insulin::TParameters &parameters = scgms::Convert_Parameters<lgs_basal_insulin::TParameters>(params, lgs_basal_insulin::default_parameters);

	const double historyTimeStep = scgms::One_Minute * 5.0;	// 5 minute step
	const size_t historyTimeCnt = static_cast<size_t>(parameters.suspend_duration / historyTimeStep);

	std::vector<double> htimes(historyTimeCnt);

	for (size_t i = 0; i < count; i++) {

		for (size_t p = 0; p < historyTimeCnt; p++) {
			htimes[historyTimeCnt - p - 1] = times[i] - static_cast<double>(p) * historyTimeStep;
		}

		bool below = false;
		std::vector<double> sensor_readings(historyTimeCnt);
		if (mIG->Get_Continuous_Levels(nullptr, htimes.data(), sensor_readings.data(), historyTimeCnt, scgms::apxNo_Derivation) == S_OK) {		

			for (size_t p = 0; p < historyTimeCnt; p++) {
				if (!std::isnan(sensor_readings[p]) && (sensor_readings[p] < parameters.suspend_threshold)) {
					below = true;
					break;
				}
			}
		}

		// suspend infusion when CGM readings are below set lower threshold
		levels[i] = below ? 0.0 : parameters.basal_insulin_rate;
	}

	return S_OK;
}

HRESULT IfaceCalling CConstant_Basal_LGS_Insulin_Rate_Model::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(lgs_basal_insulin::default_parameters);
	return parameters->set(params, params + lgs_basal_insulin::param_count);
}
