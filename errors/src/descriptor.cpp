#pragma once

#include "descriptor.h"
#include "errors.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace errors
{
	constexpr size_t param_count = 0;

	const glucose::TFilter_Descriptor Errors_Descriptor = {
		{ 0x4a125499, 0x5dc8, 0x128e, { 0xa5, 0x5c, 0x14, 0x22, 0xbc, 0xac, 0x10, 0x74 } }, //// {4A125499-5DC8-128E-A55C-1422BCAC1074}
		dsErrors_Filter,
		param_count,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { errors::Errors_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == errors::Errors_Descriptor.id)
		return Manufacture_Object<CErrors_Filter>(filter, input, output);

	return ENOENT;
}
