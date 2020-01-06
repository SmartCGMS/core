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

#include "Measured_Signal.h"

#undef min

#include <algorithm>

CMeasured_Signal::CMeasured_Signal(): mApprox(nullptr) {
	// TODO: proper approximator configuration
	//		 now we just pick the first one, which is obviously wrong

	const auto approx_descriptors = scgms::get_approx_descriptors();

	// TODO: passing approximation parameters through architecture to Approximate method
	//		 for now, we just send nullptr so the approximation method uses default parameters

	if (!approx_descriptors.empty()) {

		scgms::ISignal* self_signal = static_cast<scgms::ISignal*>(this);
		scgms::SApprox_Parameters_Vector params;

		mApprox = scgms::Create_Approximator(approx_descriptors[0].id, self_signal, params);
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

HRESULT IfaceCalling CMeasured_Signal::Get_Discrete_Bounds(scgms::TBounds* const time_bounds, scgms::TBounds* const level_bounds, size_t *level_count) const
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

HRESULT IfaceCalling CMeasured_Signal::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const {
	if (count == 0) return S_FALSE;
	if ((times == nullptr) || (levels == nullptr)) return E_INVALIDARG;
	return mApprox ? mApprox->GetLevels(times, levels, count, derivation_order) : E_FAIL;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const
{
	return E_NOTIMPL;
}
