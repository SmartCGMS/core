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

#include "Diffusion_Prediction.h"

#include "../descriptor.h"

#include "../../../../common/rtl/SolverLib.h"

#include <cmath>

CDiffusion_Prediction::CDiffusion_Prediction(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment), mIst(segment.Get_Signal(scgms::signal_IG)) {
	if (!mIst) throw std::exception{};
}


HRESULT IfaceCalling CDiffusion_Prediction::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	diffusion_prediction::TParameters &parameters = scgms::Convert_Parameters<diffusion_prediction::TParameters>(params, diffusion_prediction::default_parameters);

	//destination times
	Eigen::Map<TVector1D> converted_times{ Map_Double_To_Eigen<TVector1D>(times, count) };
	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };

	//1. we calculate retrospective BG		
	auto retrospective_present_ist = Reserve_Eigen_Buffer(mRetrospective_Present_Ist, count);
	auto &retrospective_future_ist = converted_levels;
	auto retrospective_dt = Reserve_Eigen_Buffer(mRetrospective_Dt, count);

	//converted_times is the future we want to calculate
	//=>therefore, we need to calculate BG back in time by both retro- and pred- dts
	//and use converted_times - pred.dt

	retrospective_dt = converted_times - parameters.predictive.dt;	
	HRESULT rc = mIst->Get_Continuous_Levels(nullptr, retrospective_dt.data(), retrospective_future_ist.data(), count, scgms::apxNo_Derivation);
	if (rc != S_OK) return rc;
	retrospective_dt -= parameters.retrospective.dt;
	rc = mIst->Get_Continuous_Levels(nullptr, retrospective_dt.data(), retrospective_present_ist.data(), count, scgms::apxNo_Derivation);
	if (rc != S_OK) return rc;
	
	

	if (parameters.retrospective.cg != 0.0) {		
		auto beta = Reserve_Eigen_Buffer(mBeta, count);
		beta = parameters.retrospective.p - parameters.retrospective.cg * retrospective_present_ist;	
		converted_levels = beta.square() - 4.0*parameters.retrospective.cg*(parameters.retrospective.c - retrospective_future_ist);
		for (size_t i = 0; i < count; i++)
			if (converted_levels[i] < 0.0) converted_levels[i] = 0.0;	//max would be nice solution, but it fails for some reason
		converted_levels = (converted_levels.sqrt() - beta)*0.5 / parameters.retrospective.cg;
	} else {
		if (parameters.retrospective.p != 0.0) 
			converted_levels = (retrospective_future_ist - parameters.retrospective.c) / parameters.retrospective.p; 
			else converted_levels = parameters.retrospective.c;
	}
	//Now, converted_levels hold BG at times-parameters.retrospective.dt - parameters.predictive.dt

	//2. from that BG, we calculate future IG
	const auto &retro_present_BG = converted_levels;
	converted_levels =  parameters.predictive.p*retro_present_BG
					  + parameters.predictive.cg*retro_present_BG*(retro_present_BG - retrospective_present_ist)
					  + parameters.predictive.c;
	
	return S_OK;
}

HRESULT IfaceCalling CDiffusion_Prediction::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(diffusion_prediction::default_parameters);
	return parameters->set(params, params + diffusion_prediction::param_count);
}
