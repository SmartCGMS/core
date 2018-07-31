#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"

namespace db_reader {
	constexpr GUID filter_id = { 0xc0e942b9, 0x3928, 0x4b81, { 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0x42 } }; //// {C0E942B9-3928-4B81-9B43-A34766820042}
}

namespace db_writer {
	constexpr GUID filter_id = { 0x9612e8c4, 0xb841, 0x12f7,{ 0x8a, 0x1c, 0x76, 0xc3, 0x49, 0xa1, 0x62, 0x08 } }; //// {9612E8C4-B841-12F7-8A1C-76C349A16208}
}

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter);
