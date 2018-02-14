#include "Diffusion_v2_blood.h"

#include "descriptor.h"

#include "pool.h"

#undef max

CDiffusion_v2_blood::CDiffusion_v2_blood(glucose::WTime_Segment segment) : mIst(segment.Get_Signal(glucose::signal_IG)) {
	
}


HRESULT IfaceCalling CDiffusion_v2_blood::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
	const double *times, const double *levels, const size_t count, const size_t derivation_order) const {

	HRESULT rc;
	double *begin, *end;
	if (params) {
		rc = params->get(&begin, &end);
		if (rc != S_OK) return rc;
	}
	else {
		rc = S_OK;
		begin = const_cast<double*>(&diffusion_v2_model::default_parameters[0]);
	}
	diffusion_v2_model::TParameters &parameters = *(reinterpret_cast<diffusion_v2_model::TParameters*>(begin));
	
	CPooled_Buffer<TVector1D> present_ist{ Vector1D_Pool, count };
	rc = mIst->Get_Continuous_Levels(nullptr, times, present_ist.element.data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;


	CPooled_Buffer<TVector1D> future_ist{ Vector1D_Pool, count };
	CPooled_Buffer<TVector1D> dt{ Vector1D_Pool, count };
	Eigen::Map<TVector1D> converted_times{ const_cast<double*>(times), Eigen::NoChange, static_cast<Eigen::Index>(count) };
	Eigen::Map<TVector1D> converted_levels{ const_cast<double*>(levels), Eigen::NoChange, static_cast<Eigen::Index>(count) };


	
	
	if ((parameters.k != 0.0) && (parameters.h != 0.0)) {
		//reuse dt to obtain h_back_ist		
		dt.element = converted_times - parameters.h;

		auto &h_back_ist = future_ist;	//temporarily re-use this buffer to calculate dt vector
		rc = mIst->Get_Continuous_Levels(nullptr, dt.element.data(), h_back_ist.element.data(), count, glucose::apxNo_Derivation);
		if (rc != S_OK) return rc;

		dt.element = converted_times + parameters.dt + parameters.k*present_ist.element*(present_ist.element - h_back_ist.element)/parameters.h;
	}
	else
		dt.element = converted_times + parameters.dt;

	
	rc = mIst->Get_Continuous_Levels(nullptr, dt.element.data(), future_ist.element.data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	
	
	
	
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
		auto &beta = present_ist.element;
		//auto &gamma = future_ist.element;
		//auto &D = dt.element;


		//non-zero alpha implies non-zero beta and gamma, because they do linear regression on ist with non-zero k-parameter
		//note that zero ist would mean long-term dead body

		//gamma = parameters.c - future_ist.element;
		beta = parameters.p - parameters.cg * present_ist.element;
		//D = beta * beta - 4.0*alpha*gamma;	-- no need to stora gamma for just one expression
		//D = beta * beta - 4.0*alpha*(parameters.c - future_ist.element);	-- will expand the final equation later, at the end

		//now, we have a vector of D, with some possibly negative values 
		//=> we apply maximum to reset each negative D to zero, thus neglecting the imaginary part of the square root
		//as we have not idea what to do with such a result anyway
		//D = D.max(0.0); -- will expand the final equation later, at the end


		//now, we have non-negative D and non-zero alpha => let's calculate blood
		//converted_levels = (D.sqrt() - beta)*0.5 / alpha; -- will expand the final equation later, at the end

		converted_levels = ((beta.square() - 4.0*parameters.cg*(parameters.c - future_ist.element)).max(0.0).sqrt() - beta)*0.5 / parameters.cg;
	} else {
		//with alpha==0.0 we cannot calculate discriminant D as it would lead to division by zero(alpha)		
		//=> BG = -gamma/beta but only if beta != 0.0
		//as beta degrades to params.p with zero alpha, we only need to check params.p for non-zero value
		if (parameters.p != 0.0) {
			//gamma = parameters.c - future_ist.element;
			//converted_levels = gamma / parameters.p; --let's rather expand to full experesion
			converted_levels = (parameters.c - future_ist.element) / parameters.p; //and that's it - we have degraded to linear regression
		}


		// In this case, alpha is zero, beta is zero => there's nothing more we could do.
		// By all means, of the orignal equation, the glucose level is constant,
		// thus the subject is dead => thus NaN, in-fact, would be the correct value.
		// However, let us return the constat glucose level instead.
				
		 else converted_levels = parameters.c;
	}

	return S_OK;
}

HRESULT IfaceCalling CDiffusion_v2_blood::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) {
	return parameters->set(diffusion_v2_model::default_parameters, diffusion_v2_model::default_parameters + diffusion_v2_model::param_count);
}