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

#include "Diffusion_v2_blood.h"

#include "../descriptor.h"

#include <scgms/rtl/SolverLib.h>

#include <stdexcept>

#undef max

thread_local TVector1D CDiffusion_v2_blood::mPresent_Ist, CDiffusion_v2_blood::mFuture_Ist, CDiffusion_v2_blood::mDt;

CDiffusion_v2_blood::CDiffusion_v2_blood(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment), mIst(segment.Get_Signal(scgms::signal_IG)) {
	if (!refcnt::Shared_Valid_All(mIst)) {
		throw std::runtime_error{ "Cannot find IG signal" };
	}
}

HRESULT IfaceCalling CDiffusion_v2_blood::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
																const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	if (count == 0) {
		return S_FALSE;
	}

	if ((times == nullptr) || (levels == nullptr) || (derivation_order != scgms::apxNo_Derivation)) {
		return E_INVALIDARG;
	}

	const diffusion_v2_model::TParameters &parameters = scgms::Convert_Parameters<diffusion_v2_model::TParameters>(params, diffusion_v2_model::default_parameters);
	
	auto present_ist = Reserve_Eigen_Buffer(mPresent_Ist, count );
	HRESULT rc = mIst->Get_Continuous_Levels(nullptr, times, present_ist.data(), count, scgms::apxNo_Derivation);
	if (rc != S_OK) {
		return rc;
	}

	auto future_ist = Reserve_Eigen_Buffer(mFuture_Ist, count );
	auto dt = Reserve_Eigen_Buffer(mDt, count );
	const Eigen::Map<TVector1D> converted_times{ Map_Double_To_Eigen<TVector1D>(times, count) };
	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };

	if ((parameters.k != 0.0) && (parameters.h != 0.0)) {
		//reuse dt to obtain h_back_ist
		dt = converted_times - parameters.h;

		auto &h_back_ist = future_ist;	//temporarily re-use this buffer to calculate dt vector
		rc = mIst->Get_Continuous_Levels(nullptr, dt.data(), h_back_ist.data(), count, scgms::apxNo_Derivation);
		if (rc != S_OK) {
			return rc;
		}

		dt = converted_times + parameters.dt + parameters.k*present_ist*(present_ist - h_back_ist)/parameters.h;
	}
	else {
		dt = converted_times + parameters.dt;
	}

	rc = mIst->Get_Continuous_Levels(nullptr, dt.data(), future_ist.data(), count, scgms::apxNo_Derivation);
	if (rc != S_OK) {
		return rc;
	}

	//floattype alpha = params.cg;
	//floattype beta = params.p - alpha * presentist;
	//floattype gamma = params.c - futureist;
	//
	//floattype D = beta * beta - 4.0*alpha*gamma;
	//alpha*BG^2 + beta*BG + gamma == 0
	//BG = 0.5*(-beta+D^0.5)/alpha

	//const double &alpha = parameters.cg;
	if (parameters.cg != 0.0) { //if (alpha != 0.0) {
		//prepare aliases that re-use the already allocated buffers
		//as we won't use the buffers under their original names anymore
		//actual values will be calcualted in respective branches to avoid unnecessary calculations
		auto &beta = present_ist;
		//auto &gamma = future_ist.element;
		//auto &D = dt.element;


		//non-zero alpha implies non-zero beta and gamma, because they do linear regression on ist with non-zero k-parameter
		//note that zero ist would mean long-term dead body

		//gamma = parameters.c - future_ist.element;
		beta = parameters.p - parameters.cg * present_ist;	//!!! BEWARE - we no longer have present ist !!!
		//D = beta * beta - 4.0*alpha*gamma;	-- no need to stora gamma for just one expression
		//D = beta * beta - 4.0*alpha*(parameters.c - future_ist.element);	-- will expand the final equation later, at the end

		//now, we have a vector of D, with some possibly negative values 
		//=> we apply maximum to reset each negative D to zero, thus neglecting the imaginary part of the square root
		//as we have not idea what to do with such a result anyway
		//D = D.max(0.0); -- will expand the final equation later, at the end


		//now, we have non-negative D and non-zero alpha => let's calculate blood
		//converted_levels = (D.sqrt() - beta)*0.5 / alpha; -- will expand the final equation later, at the end

		converted_levels = beta.square() - 4.0*parameters.cg*(parameters.c - future_ist);
		//converted_levels.max(0.0);	//max cannot be inlined to make a one large equation
		for (size_t i = 0; i < count; i++) {
			if (converted_levels[i] < 0.0) {
				return E_FAIL; // shouldn't we fail rather? converted_levels[i] = 0.0;	//max would be nice solution, but it fails for some reason
			}
		}
		converted_levels = (converted_levels.sqrt() - beta)*0.5 / parameters.cg;		
	} else {
		//with alpha==0.0 we cannot calculate discriminant D as it would lead to division by zero(alpha)
		//=> BG = -gamma/beta but only if beta != 0.0
		//as beta degrades to params.p with zero alpha, we only need to check params.p for non-zero value
		if (parameters.p != 0.0) {
			//gamma = parameters.c - future_ist.element;
			//converted_levels = gamma / parameters.p; --let's rather expand to full expression
			converted_levels = (future_ist - parameters.c) / parameters.p; //and that's it - we have degraded to linear regression

			//actually, now we have collapsed into simple linear reqression, which could be a valid solution
			//if the input signal is very-close to the reference signal so that it already covers that
			//glucose dynamics that this model solves
		}


		// In this case, alpha is zero, beta is zero => there's nothing more we could do.
		// By all means, of the orignal equation, the glucose level is constant,
		// thus the subject is dead => thus NaN, in-fact, would be the correct value.
		// However, let us return the constat glucose level instead.
		else { ///converted_levels = parameters.c;
			//no, it is physiologically implausible result
			//that allows stochastics methods to collapse into invalid, singular solution
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT IfaceCalling CDiffusion_v2_blood::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(diffusion_v2_model::default_parameters);
	return parameters->set(params, params + diffusion_v2_model::param_count);
}
