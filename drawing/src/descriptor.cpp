#include "descriptor.h"
#include "drawing.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace drawing
{
	constexpr size_t param_count = 9;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptBool,
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
		dsDiagnosis_Is_Type2,
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
		rsDiagnosis_Is_Type2,
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
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
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

const std::array<glucose::TFilter_Descriptor, 1> filter_descriptions = { drawing::Drawing_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == drawing::Drawing_Descriptor.id)
		return Manufacture_Object<CDrawing_Filter>(filter, input, output);

	return ENOENT;
}
