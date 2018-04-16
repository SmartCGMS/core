#pragma once

#include "descriptor.h"
#include "calculate.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace calculate
{
	constexpr size_t param_count = 3;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptModel_Id,
		glucose::NParameter_Type::ptModel_Signal_Id,
		glucose::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSelected_Model,
		dsSelected_Signal,
		dsCalculate_Past_New_Params
	};

	const wchar_t* config_param_name[param_count] = {
		rsSelected_Model,
		rsSelected_Signal,
		rsCalculate_Past_New_Params
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor Calculate_Descriptor = {
		{ 0x14a25f4c, 0xe1b1, 0x85c4, { 0x12, 0x74, 0x9a, 0x0d, 0x11, 0xe0, 0x98, 0x13 } }, //// {14A25F4C-E1B1-85C4-1274-9A0D11E09813}
		dsCalculate_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { calculate::Calculate_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == calculate::Calculate_Descriptor.id)
		return Manufacture_Object<CCalculate_Filter>(filter, input, output);

	return ENOENT;
}
