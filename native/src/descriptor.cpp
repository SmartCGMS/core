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

#include <lang/dstrings.h>

namespace native {
	const wchar_t* rsRequired_Signal_Prefix = L"Required_Signal_";

	const wchar_t* rsRequired_Signal_2 = L"Required_Signal_2";
	const wchar_t* rsRequired_Signal_3 = L"Required_Signal_3";
	const wchar_t* rsRequired_Signal_4 = L"Required_Signal_4";
	const wchar_t* rsRequired_Signal_5 = L"Required_Signal_5";
	const wchar_t* rsRequired_Signal_6 = L"Required_Signal_6";
	const wchar_t* rsRequired_Signal_7 = L"Required_Signal_7";
	const wchar_t* rsRequired_Signal_8 = L"Required_Signal_8";
	const wchar_t* rsRequired_Signal_9 = L"Required_Signal_9";
	const wchar_t* rsRequired_Signal_10 = L"Required_Signal_10";

	const wchar_t* rsEnvironment_Init = L"Environment_Init";
	const wchar_t* rsCompiler_Name = L"Compiler_Name";
	const wchar_t* rsSource_Filepath = L"Source_File";


	constexpr size_t param_count = required_signal_count+	//five required signals, of which the first one is sync signal
								  +1	//separator
								  +3;	//env init, compiler, src file path

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,

		scgms::NParameter_Type::ptNull,

		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array		
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSynchronization_Signal,
		L"Required signal 2",
		L"Required signal 3",
		L"Required signal 4",
		L"Required signal 5",
		L"Required signal 6",
		L"Required signal 7",
		L"Required signal 8",
		L"Required signal 9",
		L"Required signal 10",
		nullptr,
		L"Environment init batch",
		L"Compiler",
		L"Source file"
	};

	const wchar_t* config_param_name[param_count] = {
		rsSynchronize_to_Signal,
		rsRequired_Signal_2,
		rsRequired_Signal_3,
		rsRequired_Signal_4,
		rsRequired_Signal_5,
		nullptr,
		rsEnvironment_Init,
		rsCompiler_Name,
		rsSource_Filepath
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr
	};

	const scgms::TFilter_Descriptor desc = {
		native_filter_id,
		scgms::NFilter_Flags::None,
		L"Native Scripting",
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
	
}

const std::array<const scgms::TFilter_Descriptor, 1> filter_descriptions = { { native::desc} };

HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	*begin = const_cast<scgms::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {


	//if (*id == pattern_prediction::filter_id)
//		return Manufacture_Object<CPattern_Prediction_Filter>(filter, output);

	return E_NOTIMPL;
}
