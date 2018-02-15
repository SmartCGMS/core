#include "Diffusion_v2_ist.h"

#include "descriptor.h"
#include "pool.h"

#include <nlopt.hpp>

#undef max

namespace internal {
	double NLOpt_Objective_Function(unsigned, const double *estimated_time, double *, void *reference_time) {
		return fabs(*(static_cast<double*>(reference_time)) - *estimated_time);
	}
}

CDiffusion_v2_ist::CDiffusion_v2_ist(glucose::WTime_Segment segment) :CDiffusion_v2_blood(segment), mBlood(segment.Get_Signal(glucose::signal_BG)) {
	if (refcnt::Shared_Valid_All(mBlood)) throw std::exception{};
}


HRESULT IfaceCalling CDiffusion_v2_ist::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
	const double *times, const double *levels, const size_t count, const size_t derivation_order) const {

	diffusion_v2_model::TParameters &parameters = Convert_Parameters<diffusion_v2_model::TParameters>(params, diffusion_v2_model::default_parameters);

	Eigen::Map<TVector1D> converted_times{ const_cast<double*>(times), Eigen::NoChange, static_cast<Eigen::Index>(count) };
	Eigen::Map<TVector1D> converted_levels{ const_cast<double*>(levels), Eigen::NoChange, static_cast<Eigen::Index>(count) };
	CPooled_Buffer<TVector1D> dt = Vector1D_Pool.pop( count );

	//into the dt vector, we put times to get blood and ist to calculate future ist aka levels at the future times
	dt.element() = converted_times - parameters.dt;
	if ((parameters.k != 0.0) && (parameters.h != 0.0)) {
		//here comes the tricky part that the future distances will vary in the dt vector

		const double kh = parameters.k / parameters.h;
		
		//future_time = present_time + delta + kh*(present_ist - h_back_ist)
		//future_time, dt and kh are known => we need to solve this
		//current value of dt = present_time + kh*(present_ist - h_back_ist)

		//we give up the SIMD optimization due to unkown memory requirements
		for (size_t i = 0; i < count; i++) {
			//This for cycle could be paralelized with threads, but...
			//	..if called by solver, then may threads are already executing this function thus making this idea pointless - just increasing the scheduling overhead
			//	..if called just to calculate the levels, then single core is fast enough for the end user not to notice the difference

			auto estimate_future_time = [times, kh, this, &parameters](const double present_time)->double {
				const double ist_times[2] = { present_time - parameters.h, present_time };
				double ist_levels[2];
				if (mIst->Get_Continuous_Levels(nullptr, ist_times, ist_levels, 2, glucose::apxNo_Derivation) != S_OK) return std::numeric_limits<double>::quiet_NaN();
				if (isnan(ist_levels[0]) || isnan(ist_levels[1])) return std::numeric_limits<double>::quiet_NaN();

				return present_time + parameters.dt + kh*ist_levels[1]*(ist_levels[1] - ist_levels[0]);
			};

			//we need to minimize the difference between times[i] and estimate_future_time(estimated_present_time), whereas estimated_present_time we try to determine using some other algorithm
			//we could either try to (recursively with increasing detail) step through all the combinations, or try more sophisticated algoritm - e.g., NewUOA
			//we chose NewUOA as the number of instruction could be approximately the same and NewUOA is more intelligent approach than brute force search, even recursive one

			double minf;			
			std::vector<double> estimated_present_time(1);

			nlopt::opt opt(nlopt::LN_NEWUOA, 1); //just one double
			opt.set_min_objective(&internal::NLOpt_Objective_Function, const_cast<double*>(&times[i]));
			opt.set_lower_bounds(dt.element()[i] - glucose::One_Hour);
			opt.set_upper_bounds(dt.element()[i] + glucose::One_Hour);
			opt.set_xtol_rel(0.1*glucose::One_Second);
			nlopt::result result = opt.optimize(estimated_present_time, minf); 

			dt.element()[i] = estimated_present_time[0];
		}

		//by now, all dt elements should be estimated
	}


	CPooled_Buffer<TVector1D> present_blood = Vector1D_Pool.pop(count );
	HRESULT rc = mBlood->Get_Continuous_Levels(nullptr, dt.element().data(), present_blood.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	CPooled_Buffer<TVector1D> present_ist = Vector1D_Pool.pop ( count );
	rc = mIst->Get_Continuous_Levels(nullptr, dt.element().data(), present_ist.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;


	converted_levels =  parameters.p*present_blood.element()
		              + parameters.cg*present_blood.element()*(present_blood.element() - present_ist.element())
					  + parameters.c;

	return S_OK;
}