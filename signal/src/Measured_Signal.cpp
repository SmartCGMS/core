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

#include "Measured_Signal.h"
#include "descriptor.h"



#undef min

#include <algorithm>
#include <assert.h>


CMeasured_Signal::CMeasured_Signal(): mApprox(nullptr) {
	// TODO: proper approximator configuration
	//		 now we just pick the first one, which is obviously wrong
	
	const auto approx_descriptors = glucose::get_approx_descriptors();

	// TODO: passing approximation parameters through architecture to Approximate method
	//		 for now, we just send nullptr so the approximation method uses default parameters
	
	if (!approx_descriptors.empty()) {

		glucose::ISignal* self_signal = static_cast<glucose::ISignal*>(this);
		glucose::SApprox_Parameters_Vector params;

		mApprox = glucose::Create_Approximator(approx_descriptors[0].id, self_signal, params);		
	}
}

HRESULT IfaceCalling CMeasured_Signal::Get_Discrete_Levels(double* const times, double* const levels, const size_t count, size_t *filled) const {

	*filled = std::min(count, mTimes.size());

	if (*filled) {
		const size_t bytes_to_copy = (*filled) * sizeof(double);
		memcpy(times, mTimes.data(), bytes_to_copy);
		memcpy(levels, mLevels.data(), bytes_to_copy);
	}

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Discrete_Bounds(glucose::TBounds* const time_bounds, glucose::TBounds* const level_bounds, size_t *level_count) const
{

	if (level_count)
		*level_count = mLevels.size();

	if (mLevels.size() == 0)
		return S_FALSE;

	if (time_bounds)
	{
		time_bounds->Min = mTimes[0];
		time_bounds->Max = mTimes[mTimes.size() - 1];
	}

	if (level_bounds)
	{
		auto res = std::minmax_element(mLevels.begin(), mLevels.end());
		level_bounds->Min = *res.first;
		level_bounds->Max = *res.second;
	}

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Add_Levels(const double *times, const double *levels, const size_t count) {
	// copy given values to internal vectors
	std::copy(times, times + count, std::back_inserter(mTimes));
	std::copy(levels, levels + count, std::back_inserter(mLevels));

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const {	
	assert(times != nullptr);
	assert(levels != nullptr);
	assert(count > 0);
	return mApprox ? mApprox->GetLevels(times, levels, count, derivation_order) : E_FAIL;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const
{
	return E_NOTIMPL;
}
