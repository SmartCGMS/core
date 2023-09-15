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

#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"
#include "../../../common/rtl/ModelsLib.h"


namespace native {
	constexpr GUID native_filter_id = { 0x2abad468, 0x1708, 0x48cc, { 0xa1, 0x7a, 0x64, 0xab, 0xbe, 0xc8, 0x73, 0x27 } };	// {2ABAD468-1708-48CC-A17A-64ABBEC87327}	

	extern const wchar_t* rsRequired_Signal_Prefix;	

	extern const wchar_t* rsEnvironment_Init;
	extern const wchar_t* rsSource_Filepath;
	extern const wchar_t* rsCompiler_Name;	
	extern const wchar_t* rsCustom_Compile_Options;
	extern const wchar_t* rsSmartCGMS_Include_Dir;

	extern const char* rsScript_Entry_Symbol;
	extern const char* rsCustom_Data_Size;
}