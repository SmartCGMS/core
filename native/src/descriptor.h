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

#pragma once

#include <iface/UIIface.h>
#include <rtl/hresult.h>
#include <rtl/ModelsLib.h>


namespace native {
	constexpr GUID native_filter_id = { 0x2abad468, 0x1708, 0x48cc, { 0xa1, 0x7a, 0x64, 0xab, 0xbe, 0xc8, 0x73, 0x27 } };	// {2ABAD468-1708-48CC-A17A-64ABBEC87327}		

	

	extern const wchar_t* rsRequired_Signal_Prefix;	

	extern const wchar_t* rsEnvironment_Init;
	extern const wchar_t* rsSource_Filepath;
	extern const wchar_t* rsCompiler_Name;	
	extern const wchar_t* rsCustom_Compile_Options;
	extern const wchar_t* rsSmartCGMS_Include_Dir;

	extern const char* rsScript_Entry_Symbol;
}

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter);
