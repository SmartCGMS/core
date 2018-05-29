#include "Diffusion_v2_blood.h"

#include "descriptor.h"

#undef max

CDiffusion_v2_blood::CDiffusion_v2_blood(glucose::WTime_Segment segment) : CCommon_Calculation(segment, glucose::signal_BG), mIst(segment.Get_Signal(glucose::signal_IG)) {
	if (!refcnt::Shared_Valid_All(mIst)) throw std::exception{};
}


HRESULT IfaceCalling CDiffusion_v2_blood::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
																const double* times, double* const levels, const size_t count, const size_t derivation_order) const {
	assert((times != nullptr) && (levels != nullptr) && (count>0));

	diffusion_v2_model::TParameters &parameters = Convert_Parameters<diffusion_v2_model::TParameters>(params, diffusion_v2_model::default_parameters);
	
	CPooled_Buffer<TVector1D> present_ist = mVector1D_Pool.pop( count );
	HRESULT rc = mIst->Get_Continuous_Levels(nullptr, times, present_ist.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;


	CPooled_Buffer<TVector1D> future_ist = mVector1D_Pool.pop( count );
	CPooled_Buffer<TVector1D> dt = mVector1D_Pool.pop( count );
	Eigen::Map<TVector1D> converted_times{ Map_Double_To_Eigen<TVector1D>(times, count) };
	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };


	
	auto SolveBloodLevelDiffusion2 = [&parameters](double presentist, double futureist) {
		if (isnan(presentist) || isnan(futureist)) return std::numeric_limits<double>::quiet_NaN();
		double blood;

		double alpha = parameters.cg;
		double beta = parameters.p - alpha * presentist;
		double gamma = parameters.c - futureist;

		double D = beta * beta - 4.0*alpha*gamma;


		bool nzalpha = alpha != 0.0;
		if ((D >= 0) && (nzalpha)) {
			blood = (-beta + sqrt(D))*0.5 / alpha;
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
					blood = parameters.c;
			}	 // if (nzalpha)									
		}			 //if D>=0 & alpha != 0

		
		return blood;
	};



	
	if ((parameters.k != 0.0) && (parameters.h != 0.0)) {
		//reuse dt to obtain h_back_ist		
		dt.element() = converted_times - parameters.h;

		auto &h_back_ist = future_ist;	//temporarily re-use this buffer to calculate dt vector
		rc = mIst->Get_Continuous_Levels(nullptr, dt.element().data(), h_back_ist.element().data(), count, glucose::apxNo_Derivation);
		if (rc != S_OK) return rc;

		dt.element() = converted_times + parameters.dt + parameters.k*present_ist.element()*(present_ist.element() - h_back_ist.element())/parameters.h;
	}
	else
		dt.element() = converted_times + parameters.dt;

	
	rc = mIst->Get_Continuous_Levels(nullptr, dt.element().data(), future_ist.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	
	
	auto pres = present_ist.element();
	auto fut = future_ist.element();
		
	for (size_t i = 0; i < count; i++) {
		levels[i] = SolveBloodLevelDiffusion2(pres[i], fut[i]);
	}

	return S_OK;


	
	
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
		auto &beta = present_ist.element();
		//auto &gamma = future_ist.element;
		//auto &D = dt.element;


		//non-zero alpha implies non-zero beta and gamma, because they do linear regression on ist with non-zero k-parameter
		//note that zero ist would mean long-term dead body

		//gamma = parameters.c - future_ist.element;
		beta = parameters.p - parameters.cg * present_ist.element();
		//D = beta * beta - 4.0*alpha*gamma;	-- no need to stora gamma for just one expression
		//D = beta * beta - 4.0*alpha*(parameters.c - future_ist.element);	-- will expand the final equation later, at the end

		//now, we have a vector of D, with some possibly negative values 
		//=> we apply maximum to reset each negative D to zero, thus neglecting the imaginary part of the square root
		//as we have not idea what to do with such a result anyway
		//D = D.max(0.0); -- will expand the final equation later, at the end


		//now, we have non-negative D and non-zero alpha => let's calculate blood
		//converted_levels = (D.sqrt() - beta)*0.5 / alpha; -- will expand the final equation later, at the end

		converted_levels = ((beta.square() - 4.0*parameters.cg*(parameters.c - future_ist.element())).max(0.0).sqrt() - beta)*0.5 / parameters.cg;
	} else {
		//with alpha==0.0 we cannot calculate discriminant D as it would lead to division by zero(alpha)		
		//=> BG = -gamma/beta but only if beta != 0.0
		//as beta degrades to params.p with zero alpha, we only need to check params.p for non-zero value
		if (parameters.p != 0.0) {
			//gamma = parameters.c - future_ist.element;
			//converted_levels = gamma / parameters.p; --let's rather expand to full experesion
			converted_levels = (parameters.c - future_ist.element()) / parameters.p; //and that's it - we have degraded to linear regression
		}


		// In this case, alpha is zero, beta is zero => there's nothing more we could do.
		// By all means, of the orignal equation, the glucose level is constant,
		// thus the subject is dead => thus NaN, in-fact, would be the correct value.
		// However, let us return the constat glucose level instead.
				
		 else converted_levels = parameters.c;
	}

	return S_OK;
}

HRESULT IfaceCalling CDiffusion_v2_blood::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const {
	return parameters->set(diffusion_v2_model::default_parameters, diffusion_v2_model::default_parameters + diffusion_v2_model::param_count);
}