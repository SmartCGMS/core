/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "descriptor.h"
#include "log.h"
#include "log_replay.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/utils/descriptor_utils.h"

#include <vector>

namespace logger
{
	constexpr size_t param_count = 1;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptWChar_Array		
	};

	const wchar_t* ui_param_name[param_count] = {
		dsLog_Output_File		
	};

	const wchar_t* config_param_name[param_count] = {
		rsLog_Output_File
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsLog_File_Output_Tooltip,		
	};

	const scgms::TFilter_Descriptor Log_Descriptor = {
		scgms::IID_Log_Filter,
		scgms::NFilter_Flags::None,
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
	constexpr size_t param_count = 3;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsLog_Input_File_Or_Dir,
		dsEmit_Shutdown_Msg,
		dsInterpret_Filename_As_Segment_Id
	};

	const wchar_t* config_param_name[param_count] = {
		rsLog_Output_File,
		rsEmit_Shutdown_Msg,
		rsInterpret_Filename_As_Segment_Id
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsLog_File_Input_Tooltip,
		nullptr,
		nullptr
	};

	const scgms::TFilter_Descriptor Log_Replay_Descriptor = {
		{ 0x172ea814, 0x9df1, 0x657c,{ 0x12, 0x89, 0xc7, 0x18, 0x93, 0xf1, 0xd0, 0x85 } }, //// {172EA814-9DF1-657C-1289-C71893F1D085}
		scgms::NFilter_Flags::None,
		dsLog_Filter_Replay,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

const std::array<scgms::TFilter_Descriptor, 2> filter_descriptions = { { logger::Log_Descriptor, log_replay::Log_Replay_Descriptor } };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {
	if (*id == log_replay::Log_Replay_Descriptor.id)
		return Manufacture_Object<CLog_Replay_Filter>(filter, output);
	else if (*id == logger::Log_Descriptor.id)
		return Manufacture_Object<CLog_Filter>(filter, output);

	return E_NOTIMPL;
}