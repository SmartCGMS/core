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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "log_replay_descriptor.h"

#include <scgms/lang/dstrings.h>
#include <scgms/rtl/manufactory.h>
#include <scgms/rtl/hresult.h>
#include <scgms/utils/descriptor_utils.h>

#include <vector>


namespace log_replay {
	constexpr size_t param_count = 5;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsLog_Input_File_Or_Dir,
		dsEmit_Shutdown_Msg,
		dsInterpret_Filename_As_Segment_Id,
		dsReset_Segment_Id,
		dsEmit_All_Events_Before_Shutdown,		
	};

	const wchar_t* config_param_name[param_count] = {
		rsLog_Output_File,
		rsEmit_Shutdown_Msg,
		rsInterpret_Filename_As_Segment_Id,
		rsReset_Segment_Id,
		rsEmit_All_Events_Before_Shutdown
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsLog_File_Input_Tooltip,
		nullptr,
		nullptr,
		nullptr
	};

	const scgms::TFilter_Descriptor Log_Replay_Descriptor = {
		{ 0x172ea814, 0x9df1, 0x657c,{ 0x12, 0x89, 0xc7, 0x18, 0x93, 0xf1, 0xd0, 0x85 } }, // {172EA814-9DF1-657C-1289-C71893F1D085}
		scgms::NFilter_Flags::None,
		dsLog_Filter_Replay,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}
