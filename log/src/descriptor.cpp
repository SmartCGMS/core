#include "descriptor.h"
#include "log.h"
#include "log_replay.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/descriptor_utils.h"

#include <vector>

namespace logger
{
	constexpr size_t param_count = 1;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptWChar_Container
	};

	const wchar_t* ui_param_name[param_count] = {
		dsLog_Output_File
	};

	const wchar_t* config_param_name[param_count] = {
		rsLog_Output_File
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsLog_File_Output_Tooltip
	};

	const glucose::TFilter_Descriptor Log_Descriptor = {
		glucose::Log_Filter,
		dsLog_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace log_replay
{
	constexpr size_t param_count = 1;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptWChar_Container
	};

	const wchar_t* ui_param_name[param_count] = {
		dsLog_Output_File
	};

	const wchar_t* config_param_name[param_count] = {
		rsLog_Output_File
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsLog_File_Input_Tooltip
	};

	const glucose::TFilter_Descriptor Log_Replay_Descriptor = {
		{ 0x172ea814, 0x9df1, 0x657c,{ 0x12, 0x89, 0xc7, 0x18, 0x93, 0xf1, 0xd0, 0x85 } }, //// {172EA814-9DF1-657C-1289-C71893F1D085}
		dsLog_Filter_Replay,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

const std::array<glucose::TFilter_Descriptor, 2> filter_descriptions = { logger::Log_Descriptor, log_replay::Log_Replay_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == logger::Log_Descriptor.id)
		return Manufacture_Object<CLog_Filter>(filter, input, output);
	else if (*id == log_replay::Log_Replay_Descriptor.id)
		return Manufacture_Object<CLog_Replay_Filter>(filter, input, output);

	return ENOENT;
}
