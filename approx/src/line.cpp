#include "line.h"


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
		mSlopes.resize(valCount - 1);
	}
	else // valCount == oldCount, no need to update
		return;

	size_t filled;
	mSignal.Get_Discrete_Levels(mInputTimes.data(), mInputLevels.data(), valCount, &filled);

	// calculate slopes
	for (size_t i = 0; i < valCount - 1; i++)
		mSlopes[i] = (mInputLevels[i + 1] - mInputLevels[i]) / (mInputTimes[i + 1] - mInputTimes[i]);
}

HRESULT IfaceCalling CLine_Approximator::GetLevels(const double* times, double* const levels, const size_t count, size_t derivation_order)
{
	Update();

	// needs to call Approximate first
	if (mSlopes.empty() )
		return E_FAIL;	

	// size of mSlopes is lower by 1 than mInputTimes/Levels, and we know how to approximate just in this range
	for (size_t i = 0; i< count; i++) {

		auto iter = std::upper_bound(mInputTimes.begin(), mInputTimes.end(), times[i]);
		size_t knotIndex = std::distance(mInputTimes.begin(), iter) - 1;

		if (knotIndex < mSlopes.size()) {

			switch (derivation_order) {
				case glucose::apxNo_Derivation:
						levels[i] = mSlopes[knotIndex]*(times[i] - mInputTimes[knotIndex]) + mInputLevels[knotIndex];
						break;

				case glucose::apxFirst_Order_Derivation:
						levels[i] = mSlopes[knotIndex];
						break;

				default: levels[i] = 0.0;
						break;
			}
		}

		else if (knotIndex == mSlopes.size()) {
			switch (derivation_order) {
				case glucose::apxNo_Derivation:
					levels[i] = mInputLevels[knotIndex];
					break;

				case glucose::apxFirst_Order_Derivation:
					levels[i] = mSlopes[knotIndex-1];
					break;

				default: levels[i] = 0.0;
					break;
			}
		}
		else
			levels[i] = std::numeric_limits<double>::quiet_NaN();
	}

	return S_OK;
}
