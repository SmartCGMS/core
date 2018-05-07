#include "line.h"

#include <assert.h>

CLine_Approximator::CLine_Approximator(glucose::WSignal signal, glucose::IApprox_Parameters_Vector* configuration)
	: mSignal(signal)
{
	Update();
}

void CLine_Approximator::Update() {
	size_t oldCount = mInputTimes.size();

	size_t valCount;
	if (mSignal.Get_Discrete_Bounds(nullptr, &valCount) != S_OK)
		return;

	if (oldCount != valCount)
	{
		mInputTimes.resize(valCount);
		mInputLevels.resize(valCount);
		mSlopes.resize(valCount);
	}
	else // valCount == oldCount, no need to update
		return;

	size_t filled;
	mSignal.Get_Discrete_Levels(mInputTimes.data(), mInputLevels.data(), valCount, &filled);

	// calculate slopes
	for (size_t i = 0; i < valCount - 1; i++)
		mSlopes[i] = (mInputLevels[i + 1] - mInputLevels[i]) / (mInputTimes[i + 1] - mInputTimes[i]);
	if (valCount > 1) mSlopes[valCount - 1] = mSlopes[valCount - 2];	//special case that will reduxe GetLevels' complexity
}

HRESULT IfaceCalling CLine_Approximator::GetLevels(const double* times, double* const levels, const size_t count, size_t derivation_order) {

	assert((times != nullptr) && (levels != nullptr) && (count > 0));
	if ((times == nullptr) || (levels == nullptr)) return E_INVALIDARG;

	Update();

	// needs to call Approximate first
	if (mSlopes.empty() )
		return E_FAIL;	

	// size of mSlopes is lower by 1 than mInputTimes/Levels, and we know how to approximate just in this range
	for (size_t i = 0; i< count; i++) {
		size_t knot_index = std::numeric_limits<size_t>::max();
		if (times[i] == mInputTimes[0]) knot_index = 0;
			else if (times[i] == mInputTimes[mInputTimes.size() - 1]) knot_index = mInputTimes.size() - 1;
			else {
				std::vector<double>::iterator knot_iter = std::upper_bound(mInputTimes.begin(), mInputTimes.end(), times[i]);	
				if (knot_iter != mInputTimes.end()) knot_index = std::distance(mInputTimes.begin(), knot_iter) - 1;								
			}


		
		

			if (knot_index != std::numeric_limits<size_t>::max()) {

				switch (derivation_order) {
					case glucose::apxNo_Derivation:
						levels[i] = mSlopes[knot_index] * (times[i] - mInputTimes[knot_index]) + mInputLevels[knot_index];
						break;

					case glucose::apxFirst_Order_Derivation:
						levels[i] = mSlopes[knot_index];
						break;

					default: levels[i] = 0.0;
						break;
					}
			}
		else
			levels[i] = std::numeric_limits<double>::quiet_NaN();
	}

	return count > 0 ? S_OK : S_FALSE;
}
