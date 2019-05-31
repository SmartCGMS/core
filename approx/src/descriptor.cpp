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

#include "descriptor.h"
#include "line.h"
#include "Akima.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace line
{
	constexpr size_t param_count = 0;

	const glucose::TApprox_Descriptor LineApprox_Descriptor = {
		{ 0xb89204aa, 0x5842, 0xa8f1, { 0x4c, 0xa1, 0x43, 0x12, 0x5f, 0x4e, 0xb2, 0xa7 } },	// {B89204AA-5842-A8F1-4CA1-43125F4EB2A7}
		dsLine_Approx,
		param_count,
		nullptr,
		nullptr
	};
}

namespace akima {
	constexpr size_t param_count = 0;

	const glucose::TApprox_Descriptor Akima_Descriptor = {
		{ 0xc3e9669d, 0x594a, 0x4fd4,{ 0xb0, 0xf4, 0x44, 0xab, 0x9d, 0x4e, 0x7, 0x39 } },	// {C3E9669D-594A-4FD4-B0F4-44AB9D4E0739}
		dsAkima,
		param_count,
		nullptr,
		nullptr
	};
}

const std::array<glucose::TApprox_Descriptor, 2> approx_descriptions = { { line::LineApprox_Descriptor, akima::Akima_Descriptor } };

extern "C" HRESULT IfaceCalling do_get_approximator_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end) {
	*begin = const_cast<glucose::TApprox_Descriptor*>(approx_descriptions.data());
	*end = *begin + approx_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApprox_Parameters_Vector* configuration, glucose::IApproximator **approx) {

	// TODO: fix this; temporary construct before approximator configuration is defined - use Akima spline despite another approximator is defined
	return Manufacture_Object<CAkima>(approx, glucose::WSignal{ signal }, configuration);

	if (*approx_id == line::LineApprox_Descriptor.id)
		return Manufacture_Object<CLine_Approximator>(approx, glucose::WSignal{ signal }, configuration);
	else if (*approx_id == akima::Akima_Descriptor.id)
		return Manufacture_Object<CAkima>(approx, glucose::WSignal{ signal }, configuration);

	return ENOENT;
}
