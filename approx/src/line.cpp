#include "line.h"

#include <iostream>
#include <chrono>

CLine_Approximator::CLine_Approximator(glucose::SSignal signal, glucose::IApprox_Parameters_Vector* configuration)
	: mSignal(signal)
{
	Update();
}

void CLine_Approximator::Update()
{
	size_t oldCount = mInputTimes.cols();

	size_t valCount;
	if (mSignal->Get_Discrete_Bounds(nullptr, &valCount) != S_OK)
		return;

	if (oldCount != valCount)
	{
		mInputTimes.resize(Eigen::NoChange, valCount);
		mInputLevels.resize(Eigen::NoChange, valCount);
	}
	else // valCount == oldCount, no need to update
		return;

	size_t filled = valCount;
	mSignal->Get_Discrete_Levels(mInputTimes.data(), mInputLevels.data(), valCount, &filled);

	if (mSlopes.cols() != valCount - 1)
		mSlopes.resize(Eigen::NoChange, valCount - 1);

	// calculate slopes
	for (size_t i = 0; i < valCount - 1; i++)
		mSlopes[i] = (mInputLevels[i + 1] - mInputLevels[i]) / (mInputTimes[i + 1] - mInputTimes[i]);
}

HRESULT IfaceCalling CLine_Approximator::GetLevels(const double* times, double* const levels, const size_t count, size_t derivation_order)
{
	Update();

	// needs to call Approximate first
	if (mSlopes.cols() == 0)
		return E_FAIL;

	size_t i = 0;

	// size of mSlopes is lower by 1 than mInputTimes/Levels, and we know how to approximate just in this range
	for (size_t outIter = 0; outIter < count; outIter++)
	{
		// find first feasible index for approximation
		while (i < mSlopes.size() && times[outIter] > mInputTimes[i + 1])
			i++;

		// if the requested time is out of bounds, use constant "extrapolation" for now (constant level, zero derivation)

		switch (derivation_order)
		{
			case glucose::apxNo_Derivation:
				if (i == mSlopes.size())
					levels[outIter] = mInputLevels[i];
				else
					levels[outIter] = mInputLevels[i] + mSlopes[i] * (times[outIter] - mInputTimes[i]);
				break;
			case glucose::apxFirst_Order_Derivation:
				if (i == mSlopes.size())
					levels[outIter] = 0.0;
				else
					levels[outIter] = mSlopes[i];
				break;
			default:
				// unknown parameter value
				break;
		}
	}

	return S_OK;
}
