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
#include "drawing_filter.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include "../../../common/rtl/descriptor_utils.h"
#include <tbb/tbb_allocator.h>

#include <vector>

namespace drawing
{
	constexpr size_t param_count = 8;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container
	};

	const wchar_t* ui_param_name[param_count] = {
		dsDrawing_Filter_Canvas_Width,
		dsDrawing_Filter_Canvas_Height,
		dsDrawing_Filter_Filename_Graph,
		dsDrawing_Filter_Filename_Day,
		dsDrawing_Filter_Filename_AGP,
		dsDrawing_Filter_Filename_Parkes,
		dsDrawing_Filter_Filename_Clark,
		dsDrawing_Filter_Filename_ECDF
	};

	const wchar_t* config_param_name[param_count] = {
		rsDrawing_Filter_Canvas_Width,
		rsDrawing_Filter_Canvas_Height,
		rsDrawing_Filter_Filename_Graph,
		rsDrawing_Filter_Filename_Day,
		rsDrawing_Filter_Filename_AGP,
		rsDrawing_Filter_Filename_Parkes,
		rsDrawing_Filter_Filename_Clark,
		rsDrawing_Filter_Filename_ECDF
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsCanvas_Width_Tooltip,
		dsCanvas_Height_Tooltip,
		dsFilename_Graph_Tooltip,
		dsFilename_Day_Tooltip,
		dsFilename_AGP_Tooltip,
		dsFilename_Parkes_Tooltip,
		dsFilename_Clark_Tooltip,
		dsFilename_ECDF_Tooltip
	};

	const glucose::TFilter_Descriptor Drawing_Descriptor = {
		glucose::Drawing_Filter,
		glucose::NFilter_Flags::Synchronous,
		dsDrawing_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

static const std::vector<glucose::TFilter_Descriptor, tbb::tbb_allocator<glucose::TFilter_Descriptor>> filter_descriptions = { drawing::Drawing_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter *output, glucose::IFilter **filter) {
	if (*id == drawing::Drawing_Descriptor.id)
		return Manufacture_Object<CDrawing_Filter>(filter, output);

	return ENOENT;
}