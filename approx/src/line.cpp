/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8
 * 301 00, Pilsen
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "line.h"

#include <assert.h>
#include <cmath>
#include <algorithm>

CLine_Approximator::CLine_Approximator(scgms::WSignal signal, scgms::IApprox_Parameters_Vector* configuration)
	: mSignal(signal)
{
	Update();
}

bool CLine_Approximator::Update() {
	size_t update_count;
	if (mSignal.Get_Discrete_Bounds(nullptr, nullptr, &update_count) != S_OK)
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
		// error, we need to recalculate everything
		mInputTimes.clear();
		return false;
	}

	// calculate slopes
	for (size_t i = 0; i < update_count - 1; i++)
		mSlopes[i] = (mInputLevels[i + 1] - mInputLevels[i]) / (mInputTimes[i + 1] - mInputTimes[i]);
	if (update_count > 1) mSlopes[update_count - 1] = mSlopes[update_count - 2];	// special case that will reduxe GetLevels' complexity

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
			else if (!std::isnan(times[i])) {
				std::vector<double>::iterator knot_iter = std::upper_bound(mInputTimes.begin(), mInputTimes.end(), times[i]);
				if (knot_iter != mInputTimes.end()) knot_index = std::distance(mInputTimes.begin(), knot_iter) - 1;
			}

			if (knot_index != std::numeric_limits<size_t>::max()) {

				switch (derivation_order) {
					case scgms::apxNo_Derivation:
						levels[i] = mSlopes[knot_index] * (times[i] - mInputTimes[knot_index]) + mInputLevels[knot_index];
						break;

					case scgms::apxFirst_Order_Derivation:
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
