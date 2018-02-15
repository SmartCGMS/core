#include "Diffusion_v2_ist.h"

#include "descriptor.h"
#include "pool.h"

#undef max

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
			auto estimate_future_time = [times, kh, this, &parameters](const double present_time)->double {
				const double ist_times[2] = { present_time - parameters.h, present_time };
				double ist_levels[2];
				if (mIst->Get_Continuous_Levels(nullptr, ist_times, ist_levels, 2, glucose::apxNo_Derivation) != S_OK) return std::numeric_limits<double>::quiet_NaN();
				ist_levels[0] = ist_levels[1] - ist_levels[0];	//calculate the difference to spare one isnan test
				if (isnan(ist_levels[0])) return std::numeric_limits<double>::quiet_NaN();

				return present_time + parameters.dt + kh*ist_levels[0];
			};

			//we need to minimize the difference between times[i] and estimate_future_time(estimated_time), whereas estimated_time we try to determine using some other algorithm
		}

		return E_NOTIMPL;	//so far, we do know how to do it
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