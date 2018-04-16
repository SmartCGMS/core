#pragma once

#include "descriptor.h"
#include "net_comm.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace net_comm
{
	constexpr size_t param_count = 3;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsNet_Host,
		dsNet_Port,
		dsNet_RecvSide
	};

	const wchar_t* config_param_name[param_count] = {
		rsNet_Host,
		rsNet_Port,
		rsNet_RecvSide
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor Net_Comm_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81,{ 0x9a, 0xbc, 0x43, 0x12, 0xCC, 0x4C, 0x02, 0x98 } }, //// {C0E942B9-3928-4B81-9ABC-4312CC4C0298}
		dsNet_Comm,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { net_comm::Net_Comm_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end)
{
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == net_comm::Net_Comm_Descriptor.id)
		return Manufacture_Object<CNet_Comm>(filter, input, output);

	return ENOENT;
}
