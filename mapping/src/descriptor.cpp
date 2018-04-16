#pragma once

#include "descriptor.h"
#include "mapping.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace mapping
{
	constexpr size_t param_count = 2;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptSignal_Id,
		glucose::NParameter_Type::ptSignal_Id
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Source_Id,
		dsSignal_Destination_Id
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Source_Id,
		rsSignal_Destination_Id
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor Mapping_Descriptor = {
		{ 0x8fab525c, 0x5e86, 0xab81, { 0x12, 0xcb, 0xd9, 0x5b, 0x15, 0x88, 0x53, 0x0A } }, //// {8FAB525C-5E86-AB81-12CB-D95B1588530A}
		dsMapping_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { mapping::Mapping_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == mapping::Mapping_Descriptor.id)
		return Manufacture_Object<CMapping_Filter>(filter, input, output);

	return ENOENT;
}
