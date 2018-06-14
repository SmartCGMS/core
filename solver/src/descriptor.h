#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"

namespace compute {
	constexpr GUID solver_id = { 0xc0e942b9, 0x3928, 0x4b81,{ 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0x99 } };	//// {C0E942B9-3928-4B81-9B43-A34766820099}
}

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter);
