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
#include "native_script.h"

#include <lang/dstrings.h>
#include <utils/descriptor_utils.h>

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
	const wchar_t* rsSource_Filepath = L"Source_File";
	const wchar_t* rsCompiler_Name = L"Compiler_Name";	
	const wchar_t* rsCustom_Compile_Options = L"Custom_Compile_Options";
	const wchar_t* rsSmartCGMS_Include_Dir = L"SCMGS_Include_Dir";

	const char* rsScript_Entry_Symbol = "execute_wrapper";


	constexpr size_t param_count = max_signal_count+	//five required signals, of which the first one is sync signal
								  +1	//script parameters/double array
								  +1	//separator
								  +5;	//env init, compiler, src file path and possibly custom compiler options and SKD include

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

		scgms::NParameter_Type::ptDouble_Array,

		scgms::NParameter_Type::ptNull,

		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
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
		L"Parameters",
		nullptr,
		L"Environment init batch",
		L"Compiler",
		L"Source file",
		L"Custom compilation",
		L"SmartCGMS SDK include dir"
	};

	const wchar_t* config_param_name[param_count] = {
		rsSynchronization_Signal,
		rsRequired_Signal_2,
		rsRequired_Signal_3,
		rsRequired_Signal_4,
		rsRequired_Signal_5,
		rsRequired_Signal_6,
		rsRequired_Signal_7,
		rsRequired_Signal_8,
		rsRequired_Signal_9,
		rsRequired_Signal_10,
		rsParameters,
		nullptr,
		rsEnvironment_Init,
		rsCompiler_Name,
		rsSource_Filepath,
		rsCustom_Compile_Options,
		rsSmartCGMS_Include_Dir
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


namespace native_model {
	//This is a facade model to allow 10 generic parameters
	constexpr size_t model_param_count = 10;

	const scgms::NModel_Parameter_Value filter_param_types[model_param_count] = { scgms::NModel_Parameter_Value::mptTime,
		scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble  };

	const wchar_t* filter_param_ui_names[model_param_count] = {
		L"Parameter 1", L"Parameter 2", L"Parameter 3", L"Parameter 4", L"Parameter 5",
		L"Parameter 6", L"Parameter 7", L"Parameter 8", L"Parameter 9", L"Parameter 10" };

	const wchar_t* filter_param_config_names[model_param_count] = { 
		L"Parameter_1", L"Parameter_2", L"Parameter_3", L"Parameter_4", L"Parameter_5",
		L"Parameter_6", L"Parameter_7", L"Parameter_8", L"Parameter_9", L"Parameter_10" };


	const wchar_t* filter_param_tooltips[model_param_count] = { nullptr, nullptr, nullptr, nullptr, nullptr,
																nullptr, nullptr, nullptr, nullptr,
																nullptr};

	const double lower_bound[model_param_count] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	const double default_parameters[model_param_count] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	const double upper_bound[model_param_count] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	
	const scgms::TModel_Descriptor model_desc = {
			native::native_filter_id,
			scgms::NModel_Flags::None,
			L"Native Script Parameters",
			L"Native_Script_Parameters",

			model_param_count,
			filter_param_types,
			filter_param_ui_names,
			filter_param_config_names,

			lower_bound,
			default_parameters,
			upper_bound,

			0,
			& scgms::signal_Null,
			&scgms::signal_Null
	};	

}

const std::array<const scgms::TFilter_Descriptor, 1> filter_descriptions = { { native::desc} };
const std::array<scgms::TModel_Descriptor, 1> model_descriptions = { { native_model::model_desc} };

HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {
	if (*id == native::native_filter_id)
		return Manufacture_Object<CNative_Script>(filter, output);

	return E_NOTIMPL;
}

HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor** begin, scgms::TModel_Descriptor** end) {	
	return do_get_descriptors(model_descriptions, begin, end);
}