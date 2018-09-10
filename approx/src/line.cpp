/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "line.h"

#include <assert.h>
#include <cmath>
#include <algorithm>

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
			else if (!std::isnan(times[i])) {
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
