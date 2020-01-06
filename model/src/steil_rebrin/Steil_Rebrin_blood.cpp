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

#include "Steil_Rebrin_blood.h"

#include "../descriptor.h"

#include "../../../../common/rtl/SolverLib.h"

#include <cmath>

CSteil_Rebrin_blood::CSteil_Rebrin_blood(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment), mIst(segment.Get_Signal(scgms::signal_IG)), mCalibration(segment.Get_Signal(scgms::signal_Calibration)) {
	if (!refcnt::Shared_Valid_All(mIst, mCalibration)) throw std::exception{};
}



HRESULT IfaceCalling CSteil_Rebrin_blood::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	steil_rebrin::TParameters &parameters = scgms::Convert_Parameters<steil_rebrin::TParameters>(params, steil_rebrin::default_parameters);
	if (parameters.alpha == 0.0) return E_INVALIDARG;	//this parameter cannot be zero

	auto present_ist = Reserve_Eigen_Buffer(mPresent_Ist, count );
	HRESULT rc = mIst->Get_Continuous_Levels(nullptr, times, present_ist.data(), count, scgms::apxNo_Derivation);
	if (rc != S_OK) return rc;

	auto derived_ist = Reserve_Eigen_Buffer(mDerived_Ist, count );
	rc = mIst->Get_Continuous_Levels(nullptr, times, derived_ist.data(), count, scgms::apxFirst_Order_Derivation);
	if (rc != S_OK) return rc;

	auto calibration_offsets = Reserve_Eigen_Buffer(mCalibration_Offsets, count );
	//we need to go through the calibration vector and convert it to the time offset of the first calibrations
	//and for that we need to consider the very first BG measurement as calibration
	scgms::TBounds time_bounds;
	rc = mIst->Get_Discrete_Bounds(&time_bounds, nullptr, nullptr);
	if (rc != S_OK) return rc;

	double recent_calibration_time = time_bounds.Min;

	rc = mCalibration->Get_Continuous_Levels(nullptr, times, calibration_offsets.data(), count, scgms::apxNo_Derivation);
	if (rc != S_OK) calibration_offsets.setConstant(std::numeric_limits<double>::quiet_NaN());	//if failed, well, let's behave like if there was no calibration at all

	for (size_t i = 0; i < count; i++) {
		if (std::isnan(calibration_offsets[i])) calibration_offsets[i] = times[i] - recent_calibration_time;
			else {
				calibration_offsets[i] = 0.0;
				recent_calibration_time = times[i];
			}
	}


	//we have all the signals, let's calculate blood
	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };
	const double c = parameters.beta + parameters.tau*parameters.gamma;
	converted_levels = (parameters.tau*derived_ist + present_ist - c - parameters.gamma*calibration_offsets) / parameters.alpha;
	
	return S_OK;
}

HRESULT IfaceCalling CSteil_Rebrin_blood::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(steil_rebrin::default_parameters);
	return parameters->set(params, params + steil_rebrin::param_count);
}
