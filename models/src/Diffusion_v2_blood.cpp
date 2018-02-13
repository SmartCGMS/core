#include "Diffusion_v2_blood.h"

#include "descriptor.h"

#include "pool.h"

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

	CPooled_Buffer<TVector1D> dt{ Vector1D_Pool, count };
	Eigen::Map<TVector1D> converted_times{ const_cast<double*>(times), Eigen::NoChange, static_cast<Eigen::Index>(count) };

	if ((parameters.k != 0.0) && (parameters.h != 0.0)) {
		//reuse dt to obtain h_back_ist		
		dt.element = converted_times - parameters.h;

		CPooled_Buffer<TVector1D> h_back_ist{ Vector1D_Pool, count };
		rc = mIst->Get_Continuous_Levels(nullptr, dt.element.data(), h_back_ist.element.data(), count, glucose::apxNo_Derivation);
		if (rc != S_OK) return rc;

		dt.element = converted_times + parameters.dt + parameters.k*present_ist.element*(present_ist.element - h_back_ist.element)/parameters.h;
	}
	else
		dt.element = converted_times + parameters.dt;


	Eigen::Map<TVector1D> converted_levels{ const_cast<double*>(levels), Eigen::NoChange, static_cast<Eigen::Index>(count) };


	floattype blood;

	floattype alpha = params.cg;
	floattype beta = params.p - alpha * presentist;
	floattype gamma = params.c - futureist;

	floattype D = beta * beta - 4.0*alpha*gamma;


	bool nzalpha = alpha != 0.0;
	if ((D >= 0) && (nzalpha)) {
		blood = (-beta + params.s*sqrt(D))*0.5 / alpha;
	}
	else {
		if (nzalpha)
			blood = -beta * 0.5 / alpha;
		//OK, we should set this to NaN, but let's rather try to ignore
		//the imaginary part of the result. It is as best as it would
		//be if we would step through all possible levels.
		else {
			if (beta != 0.0)
				blood = -gamma / beta;
			else
				//blood = std::numeric_limits<floattype>::quiet_NaN();
				/* In this case, alpha is zero, beta is zero => there's nothing more we could do.
				By all means, of the orignal equation, the glucose level is constant,
				thus the subject is dead => thus NaN, in-fact, would be the correct value.
				However, let us return the constat glucose level instead.
				*/
				blood = params.c;
		}	 // if (nzalpha)									
	}			 //if D>=0 & alpha != 0


}

HRESULT IfaceCalling CDiffusion_v2_blood::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) {
	return parameters->set(diffusion_v2_model::default_parameters, diffusion_v2_model::default_parameters + diffusion_v2_model::param_count);
}