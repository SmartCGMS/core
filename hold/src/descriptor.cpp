#pragma once

#include "descriptor.h"
#include "hold.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace hold
{
	constexpr size_t param_count = 1;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptInt64
	};

	const wchar_t* ui_param_name[param_count] = {
		dsHold_Values_Delay
	};

	const wchar_t* config_param_name[param_count] = {
		rsHold_Values_Delay
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr
	};

	const glucose::TFilter_Descriptor Hold_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81, { 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0xAA } }, //// {C0E942B9-3928-4B81-9B43-A347668200AA}
		dsHold_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { hold::Hold_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == hold::Hold_Descriptor.id)
		return Manufacture_Object<CHold_Filter>(filter, input, output);

	return ENOENT;
}
