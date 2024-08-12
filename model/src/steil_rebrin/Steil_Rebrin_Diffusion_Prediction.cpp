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

#include "Steil_Rebrin_Diffusion_Prediction.h"

#include "../descriptor.h"

#include <scgms/rtl/SolverLib.h>

#include <cmath>
#include <stdexcept>

thread_local TVector1D CSteil_Rebrin_Diffusion_Prediction::mDt, CSteil_Rebrin_Diffusion_Prediction::mPresent_Ist, CSteil_Rebrin_Diffusion_Prediction::mDeriveed_Ist;

CSteil_Rebrin_Diffusion_Prediction::CSteil_Rebrin_Diffusion_Prediction(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment), mIst(segment.Get_Signal(scgms::signal_IG)) {
	if (!mIst) {
		throw std::runtime_error{ "Could not find IG signal" };
	}
}

HRESULT IfaceCalling CSteil_Rebrin_Diffusion_Prediction::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	const steil_rebrin_diffusion_prediction::TParameters &parameters = scgms::Convert_Parameters<steil_rebrin_diffusion_prediction::TParameters>(params, steil_rebrin_diffusion_prediction::default_parameters);

	Eigen::Map<TVector1D> converted_times{ Map_Double_To_Eigen<TVector1D>(times, count) };
	//into the dt vector, we put times to get ist to calculate future ist aka levels at the future times
	auto dt = Reserve_Eigen_Buffer(mDt, count);
	dt = converted_times - parameters.dt;

	auto present_ist = Reserve_Eigen_Buffer(mPresent_Ist,  count );
	HRESULT rc = mIst->Get_Continuous_Levels(nullptr, dt.data(), present_ist.data(), count, scgms::apxNo_Derivation);
	if (rc != S_OK) {
		return rc;
	}

	auto derived_ist = Reserve_Eigen_Buffer(mDeriveed_Ist, count );
	rc = mIst->Get_Continuous_Levels(nullptr, dt.data(), derived_ist.data(), count, scgms::apxFirst_Order_Derivation);
	if (rc != S_OK) {
		return rc;
	}
	
	//we have all the signals, let's calculate blood
	auto &blood = dt;	//reused old bfuffer
	blood = parameters.inv_g*parameters.tau*derived_ist + parameters.inv_g*present_ist;

	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };
	converted_levels = parameters.p*blood + parameters.cg*blood*(blood - present_ist) + parameters.c;	
	
	return S_OK;
}

HRESULT IfaceCalling CSteil_Rebrin_Diffusion_Prediction::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(steil_rebrin::default_parameters);
	return parameters->set(params, params + steil_rebrin::param_count);
}
