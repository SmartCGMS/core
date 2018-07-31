#include "Steil_Rebrin_blood.h"

#include "descriptor.h"

#include <cmath>

CSteil_Rebrin_blood::CSteil_Rebrin_blood(glucose::WTime_Segment segment) : CCommon_Calculation(segment, glucose::signal_IG), mIst(segment.Get_Signal(glucose::signal_IG)), mCalibration(segment.Get_Signal(glucose::signal_Calibration)) {
	if (!refcnt::Shared_Valid_All(mIst, mCalibration)) throw std::exception{};
}



HRESULT IfaceCalling CSteil_Rebrin_blood::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	
	steil_rebrin::TParameters &parameters = Convert_Parameters<steil_rebrin::TParameters>(params, steil_rebrin::default_parameters);
	if (parameters.alpha == 0.0) return E_INVALIDARG;	//this parameter cannot be zero
		

	CPooled_Buffer<TVector1D> ist = mVector1D_Pool.pop( count );
	HRESULT rc = mIst->Get_Continuous_Levels(nullptr, times, ist.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	CPooled_Buffer<TVector1D> derived_ist = mVector1D_Pool.pop( count );
	rc = mIst->Get_Continuous_Levels(nullptr, times, derived_ist.element().data(), count, glucose::apxFirst_Order_Derivation);
	if (rc != S_OK) return rc;

	CPooled_Buffer<TVector1D> calibration_offsets = mVector1D_Pool.pop( count );
	//we need to go through the calibration vector and convert it to the time offset of the first calibrations
	//and for that we need to consider the very first BG measurement as calibration
	glucose::TBounds bounds;
	rc = mIst->Get_Discrete_Bounds(&bounds, nullptr);
	if (rc != S_OK) return rc;

	double recent_calibration_time = bounds.Min_Time;

	rc = mCalibration->Get_Continuous_Levels(nullptr, times, calibration_offsets.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) calibration_offsets.element().setConstant(std::numeric_limits<double>::quiet_NaN());	//if failed, well, let's behave like if there was no calibration at all

	for (size_t i = 0; i < count; i++) {
		if (std::isnan(calibration_offsets.element()[i])) calibration_offsets.element()[i] = times[i] - recent_calibration_time;
			else {
				calibration_offsets.element()[i] = 0.0;
				recent_calibration_time = times[i];
			}
	}


	//we have all the signals, let's calculate blood
	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };
	converted_levels = (parameters.tau*derived_ist.element() + ist.element() - (parameters.beta + parameters.tau*parameters.gamma )- parameters.gamma*calibration_offsets.element()) / parameters.alpha;
	
	return S_OK;
}

HRESULT IfaceCalling CSteil_Rebrin_blood::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(steil_rebrin::default_parameters);
	return parameters->set(params, params + steil_rebrin::param_count);
}
