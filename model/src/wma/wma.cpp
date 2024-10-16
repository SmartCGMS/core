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

#include "wma.h"

#include "wma_descriptor.h"

#include <scgms/rtl/SolverLib.h>
#include <scgms/utils/math_utils.h>

#include <cmath>
#include <stdexcept>

thread_local TVector1D CWMA::mPresent_Ist, CWMA::mOffsets;

CWMA::CWMA(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment), mIst(segment.Get_Signal(scgms::signal_IG)) {
	if (!refcnt::Shared_Valid_All(mIst)) {
		throw std::runtime_error{ "Could not find IG signal" };
	}
}

HRESULT IfaceCalling CWMA::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	const wma::TParameters &parameters = scgms::Convert_Parameters<wma::TParameters>(params, wma::default_parameters);
	if (parameters.dt <= 0.0) {
		return E_INVALIDARG;	//this parameter must be positive
	}

	auto present_ist = Reserve_Eigen_Buffer(mPresent_Ist, wma::coeff_count);
	
	auto offsets = Reserve_Eigen_Buffer(mOffsets, wma::coeff_count);

	auto get_times = [this, &parameters, &present_ist, &times, &offsets, derivation_order](const double rattime) {
		double wdt = rattime - parameters.dt;
		for (size_t i = wma::model_param_count-1; i !=0 ; i--) {
			offsets[i] = wdt;
			wdt -= 5.0 * scgms::One_Minute;
		}

		HRESULT rc= mIst->Get_Continuous_Levels(nullptr, times, present_ist.data(), wma::coeff_count, derivation_order);
		if (rc == S_OK) {
			if (Is_Any_NaN(present_ist)) {
				rc = MK_E_UNAVAILABLE;
			}
		}

		return rc;
	};

	
	auto non_adaptive = [&parameters, &present_ist]() {
		double result = 0.0;
		for (size_t i = 0; i < wma::coeff_count; i++) {
			result += parameters.coeff[i] * present_ist[i];
		}

		return result;
	};


	for (size_t i = 0; i < count; i++) {
		if (get_times(times[i]) == S_OK) {
			levels[i] = non_adaptive();
		}
		else {
			levels[i] = std::numeric_limits<double>::quiet_NaN();
		}
	}
	
	return S_OK;
}

HRESULT IfaceCalling CWMA::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(wma::default_parameters);
	return parameters->set(params, params + wma::model_param_count);
}
