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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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
#include "filter/drawing_filter_v2.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/utils/descriptor_utils.h"

#include <array>

/*
 * Drawing filter v2 descriptor
 */

namespace drawing_filter_v2 {

	constexpr size_t param_count = 2;

	const scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptInt64
	};

	const wchar_t* ui_param_name[param_count] = {
		L"Default canvas width",
		L"Default canvas height"
	};

	const wchar_t* rsConfig_Drawing_Width = L"default_width";
	const wchar_t* rsConfig_Drawing_Height = L"default_height";

	const wchar_t* config_param_name[param_count] = {
		rsConfig_Drawing_Width,
		rsConfig_Drawing_Height
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		L"Default width of drawing canvas",
		L"Default height of drawing canvas"
	};

	const wchar_t* filter_name = L"Drawing filter v2";

	const scgms::TFilter_Descriptor descriptor = {
		scgms::IID_Drawing_Filter_v2,
		scgms::NFilter_Flags::Presentation_Only,
		filter_name,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

/*
 * Array of available filter descriptors
 */

const std::array<scgms::TFilter_Descriptor, 1> filter_descriptions = { { drawing_filter_v2::descriptor } };

/*
 * Filter library interface implementations
 */

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {

	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {

	if (*id == drawing_filter_v2::descriptor.id) {
		return Manufacture_Object<CDrawing_Filter_v2>(filter, output);
	}

	return E_NOTIMPL;
}
