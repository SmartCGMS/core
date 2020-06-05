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
#include "event_export.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/utils/descriptor_utils.h"

#include <vector>

namespace event_export
{
	constexpr const GUID id = { 0x6d3a3153, 0x67b6, 0x405c, { 0x85, 0x59, 0x7b, 0x5f, 0xaf, 0xc2, 0xeb, 0xe2 } }; // {6D3A3153-67B6-405C-8559-7B5FAFC2EBE2}

	constexpr size_t param_count = 0;

	const scgms::TFilter_Descriptor descriptor = {
		id,
		scgms::NFilter_Flags::None,
		L"Event export",
		param_count,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};
}

const std::array<scgms::TFilter_Descriptor, 1> filter_descriptions = { { event_export::descriptor } };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter)
{
	if (*id == event_export::descriptor.id)
		return Manufacture_Object<CEvent_Export_Filter>(filter, output);

	return ENOENT;
}
