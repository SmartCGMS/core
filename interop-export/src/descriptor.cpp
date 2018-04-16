#include "descriptor.h"
#include "interop-export.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace interop_export
{
	constexpr size_t param_count = 0;

	const glucose::TFilter_Descriptor Interop_Export_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81, { 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0xEE } },	//// {C0E942B9-3928-4B81-9B43-A347668200EE}
		dsInterop_Export_Filter,
		param_count,
		nullptr,
		nullptr,
		nullptr
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { interop_export::Interop_Export_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == interop_export::Interop_Export_Descriptor.id)
		return Manufacture_Object<CInterop_Export_Filter>(filter, input, output);

	return ENOENT;
}
