#include "line.h"

#include <assert.h>

CLine_Approximator::CLine_Approximator(glucose::WSignal signal, glucose::IApprox_Parameters_Vector* configuration)
	: mSignal(signal)
{
	Update();
}

bool CLine_Approximator::Update() {
	size_t update_count;
	if (mSignal.Get_Discrete_Bounds(nullptr, &update_count) != S_OK)
		return false;

	if (mInputTimes.size() != update_count) {
		mInputTimes.resize(update_count);
		mInputLevels.resize(update_count);
		mSlopes.resize(update_count);
	}
	else // valCount == oldCount, no need to update
		return true;

	size_t filled;
	if (mSignal.Get_Discrete_Levels(mInputTimes.data(), mInputLevels.data(), update_count, &filled) != S_OK) {
		mInputTimes.clear();			//error, we need to recalculat everything
		return false;
	}

	// calculate slopes
	for (size_t i = 0; i < update_count - 1; i++)
		mSlopes[i] = (mInputLevels[i + 1] - mInputLevels[i]) / (mInputTimes[i + 1] - mInputTimes[i]);
	if (update_count > 1) mSlopes[update_count - 1] = mSlopes[update_count - 2];	//special case that will reduxe GetLevels' complexity

	return true;
}

HRESULT IfaceCalling CLine_Approximator::GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) {

	assert((times != nullptr) && (levels != nullptr) && (count > 0));
	if ((times == nullptr) || (levels == nullptr)) return E_INVALIDARG;

	if (!Update() || mSlopes.empty()) return E_FAIL;


	// size of mSlopes is lower by 1 than mInputTimes/Levels, and we know how to approximate just in this range
	for (size_t i = 0; i< count; i++) {
		size_t knot_index = std::numeric_limits<size_t>::max();
		if (times[i] == mInputTimes[0]) knot_index = 0;
			else if (times[i] == mInputTimes[mInputTimes.size() - 1]) knot_index = mInputTimes.size() - 1;
			else if (!isnan(times[i])) {
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
