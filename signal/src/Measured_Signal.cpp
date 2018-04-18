#include "Measured_Signal.h"
#include "descriptor.h"



#undef min

#include <algorithm>

CMeasured_Signal::CMeasured_Signal(): mApprox(nullptr) {
	// TODO: proper approximator configuration
	//		 now we just pick the first one, which is obviously wrong
	
	const auto approx_descriptors = glucose::get_approx_descriptors();

	// TODO: passing approximation parameters through architecture to Approximate method
	//		 for now, we just send nullptr so the approximation method uses default parameters

	if (!approx_descriptors.empty())
		mApprox = glucose::Create_Approximator(approx_descriptors[0].id, refcnt::make_shared_reference_ext<glucose::SSignal, glucose::ISignal>(static_cast<glucose::ISignal*>(this), true),  glucose::SApprox_Parameters_Vector{});	
}

HRESULT IfaceCalling CMeasured_Signal::Get_Discrete_Levels(double* const times, double* const levels, const size_t count, size_t *filled) const
{
	*filled = std::min(count, mTimes.size());

	for (size_t i = 0; i < *filled; i++)
		times[i] = mTimes[i];

	for (size_t i = 0; i < *filled; i++)
		levels[i] = mLevels[i];

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Discrete_Bounds(glucose::TBounds *bounds, size_t *level_count) const
{
	if (level_count)
		*level_count = mLevels.size();

	if (bounds == nullptr)
		return S_OK;

	if (mLevels.size() == 0)
		return E_FAIL;

	bounds->Min_Time = mTimes[0];
	bounds->Max_Time = mTimes[mTimes.size() - 1];

	auto res = std::minmax_element(mLevels.begin(), mLevels.end());
	bounds->Min_Level = *res.first;
	bounds->Max_Level = *res.second;

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Add_Levels(const double *times, const double *levels, const size_t count)
{
	// reserve size, so the capacity suffices for newly inserted values
	mTimes.reserve(mTimes.size() + count);
	mLevels.reserve(mLevels.size() + count);

	// copy given values to internal vectors
	std::copy(times, times + count, std::back_inserter(mTimes));
	std::copy(levels, levels + count, std::back_inserter(mLevels));

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const
{
	return mApprox->GetLevels(times, levels, count, derivation_order);
}

HRESULT IfaceCalling CMeasured_Signal::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const
{
	return S_OK;
}
