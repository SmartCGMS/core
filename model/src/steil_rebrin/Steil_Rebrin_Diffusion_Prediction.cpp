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

#include "Steil_Rebrin_Diffusion_Prediction.h"

#include "../descriptor.h"

#include "../../../../common/rtl/SolverLib.h"

#include <cmath>

CSteil_Rebrin_Diffusion_Prediction::CSteil_Rebrin_Diffusion_Prediction(glucose::WTime_Segment segment) : CCommon_Calculed_Signal(segment), mIst(segment.Get_Signal(glucose::signal_IG)) {
	if (!mIst) throw std::exception{};
}

HRESULT IfaceCalling CSteil_Rebrin_Diffusion_Prediction::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	steil_rebrin_diffusion_prediction::TParameters &parameters = solver::Convert_Parameters<steil_rebrin_diffusion_prediction::TParameters>(params, steil_rebrin_diffusion_prediction::default_parameters);

	Eigen::Map<TVector1D> converted_times{ Map_Double_To_Eigen<TVector1D>(times, count) };
	//into the dt vector, we put times to get ist to calculate future ist aka levels at the future times
	CPooled_Buffer<TVector1D> dt = mVector1D_Pool.pop(count);
	dt.element() = converted_times - parameters.dt;

	CPooled_Buffer<TVector1D> ist = mVector1D_Pool.pop( count );
	HRESULT rc = mIst->Get_Continuous_Levels(nullptr, dt.element().data(), ist.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	CPooled_Buffer<TVector1D> derived_ist = mVector1D_Pool.pop( count );
	rc = mIst->Get_Continuous_Levels(nullptr, dt.element().data(), derived_ist.element().data(), count, glucose::apxFirst_Order_Derivation);
	if (rc != S_OK) return rc;

	
	//we have all the signals, let's calculate blood
	auto &blood = dt;	//reused old bfuffer
	blood.element() = parameters.inv_g*parameters.tau*derived_ist.element() + parameters.inv_g*ist.element();

	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };
	converted_levels = parameters.p*blood.element() + parameters.cg*blood.element()*(blood.element() - ist.element()) + parameters.c;	
	
	return S_OK;
}

HRESULT IfaceCalling CSteil_Rebrin_Diffusion_Prediction::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(steil_rebrin::default_parameters);
	return parameters->set(params, params + steil_rebrin::param_count);
}
