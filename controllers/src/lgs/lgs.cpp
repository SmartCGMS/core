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

#include "lgs.h"
#include "../descriptor.h"
#include "../../../../common/rtl/SolverLib.h"

#include <cmath>

CConstant_Basal_LGS_Insulin_Rate_Model::CConstant_Basal_LGS_Insulin_Rate_Model(glucose::WTime_Segment segment)
	: CCommon_Calculated_Signal(segment), mIG(segment.Get_Signal(glucose::signal_IG)) {
	//
}

HRESULT IfaceCalling CConstant_Basal_LGS_Insulin_Rate_Model::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const
{
	lgs_basal_insulin::TParameters &parameters = glucose::Convert_Parameters<lgs_basal_insulin::TParameters>(params, lgs_basal_insulin::default_parameters);

	const double historyTimeStep = glucose::One_Minute * 5;	// 5 minute step
	const size_t historyTimeCnt = static_cast<size_t>(parameters.suspend_duration / historyTimeStep);

	std::vector<double> htimes(historyTimeCnt);

	for (size_t i = 0; i < count; i++)
	{
		for (size_t p = 0; p < historyTimeCnt; p++)
			htimes[historyTimeCnt - p - 1] = times[i] - static_cast<double>(p)*historyTimeStep;

		std::vector<double> sensor_readings(historyTimeCnt);
		if (mIG->Get_Continuous_Levels(nullptr, htimes.data(), sensor_readings.data(), historyTimeCnt, glucose::apxNo_Derivation) != S_OK) {
			std::fill(sensor_readings.begin(), sensor_readings.end(), std::numeric_limits<double>::quiet_NaN());
		}

		bool below = false;
		for (size_t p = 0; p < historyTimeCnt; p++)
		{
			if (!std::isnan(sensor_readings[p]) && sensor_readings[p] < parameters.lower_threshold)
				below = true;
		}

		// suspend infusion when CGM readings are below set lower threshold
		levels[i] = below ? 0.0 : parameters.bin;
	}

	return S_OK;
}

HRESULT IfaceCalling CConstant_Basal_LGS_Insulin_Rate_Model::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(lgs_basal_insulin::default_parameters);
	return parameters->set(params, params + lgs_basal_insulin::param_count);
}
