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

#include "descriptor.h"
#include "drawing.h"

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

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == drawing::Drawing_Descriptor.id)
	{
		glucose::SFilter_Pipe shared_in = refcnt::make_shared_reference_ext<glucose::SFilter_Pipe, glucose::IFilter_Pipe>(input, true);
		glucose::SFilter_Pipe shared_out = refcnt::make_shared_reference_ext<glucose::SFilter_Pipe, glucose::IFilter_Pipe>(output, true);

		return Manufacture_Object<CDrawing_Filter>(filter, shared_in, shared_out);
	}

	return ENOENT;
}
