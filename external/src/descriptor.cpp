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

#ifndef NO_BUILD_DMMS
#include "dmms_lib/dmms_lib.h"
#include "dmms_proc/dmms_proc.h"
#endif
#include "network/network_model.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/UILib.h"

#ifndef NO_BUILD_DMMS
namespace dmms_model {

	const wchar_t *model_param_ui_names[model_param_count] = {
		L"Meal announce delta",
		L"Exercise announce delta",
	};

	const scgms::NModel_Parameter_Value model_param_types[model_param_count] = {
		scgms::NModel_Parameter_Value::mptTime,
		scgms::NModel_Parameter_Value::mptTime,
	};

	constexpr size_t number_of_calculated_signals = 3;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		signal_DMMS_BG,
		signal_DMMS_IG,
		signal_DMMS_Delivered_Insulin_Basal
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		L"blood glucose",
		L"interst. glucose",
		L"delivered basal insulin rate"
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_BG,
		scgms::signal_IG,
		scgms::signal_Delivered_Insulin_Basal_Rate
	};

	scgms::TModel_Descriptor proc_desc = {
		proc_model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"DMMS discrete model (process)",
		nullptr,
		model_param_count,
		model_param_types,
		model_param_ui_names,
		nullptr,
		lower_bounds.vector,
		default_parameters.vector,
		upper_bounds.vector,

		number_of_calculated_signals,
		calculated_signal_ids,
		reference_signal_ids,
	};

	scgms::TModel_Descriptor lib_desc = {
		lib_model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"DMMS discrete model (library)",
		nullptr,
		model_param_count,
		model_param_types,
		model_param_ui_names,
		nullptr,
		lower_bounds.vector,
		default_parameters.vector,
		upper_bounds.vector,

		number_of_calculated_signals,
		calculated_signal_ids,
		reference_signal_ids,
	};

	const std::wstring blood_str_desc = std::wstring{ L"DMMS discrete model" } +L" - " + dsBlood;
	const scgms::TSignal_Descriptor blood_desc{ signal_DMMS_BG, blood_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFFFF0000, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const std::wstring ig_str_desc = std::wstring{ L"DMMS discrete model" } +L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_DMMS_IG, ig_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const std::wstring delivered_insulin_str_desc = std::wstring{ L"DMMS discrete model" } +L" - delivered insulin";
	const scgms::TSignal_Descriptor delivered_insulin_desc{ signal_DMMS_Delivered_Insulin_Basal, delivered_insulin_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
}
#endif

namespace network_model {

	constexpr size_t number_of_calculated_signals = 0;

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"Network model",
		nullptr,
		model_param_count,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,

		number_of_calculated_signals,
		nullptr,
		nullptr,
	};
}

#ifndef NO_BUILD_DMMS
	constexpr size_t ExternalModelCount = 3;
#else
	constexpr size_t ExternalModelCount = 1;
#endif

const std::array<scgms::TModel_Descriptor, ExternalModelCount> model_descriptions = { {
#ifndef NO_BUILD_DMMS
	dmms_model::proc_desc,
	dmms_model::lib_desc,
#endif
	network_model::desc,
} };

#ifndef NO_BUILD_DMMS
const std::array<scgms::TSignal_Descriptor, 3> signals_descriptors = { { dmms_model::blood_desc, dmms_model::ig_desc, dmms_model::delivered_insulin_desc } };
#endif


extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	*begin = const_cast<scgms::TModel_Descriptor*>(model_descriptions.data());
	*end = *begin + model_descriptions.size();
	return S_OK;
}


extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
#ifndef NO_BUILD_DMMS
	if (*model_id == dmms_model::proc_model_id) return Manufacture_Object<CDMMS_Proc_Discrete_Model>(model, parameters, output);
	if (*model_id == dmms_model::lib_model_id) return Manufacture_Object<CDMMS_Lib_Discrete_Model>(model, parameters, output);
#endif
	if (*model_id == network_model::desc.id) return Manufacture_Object<CNetwork_Discrete_Model>(model, parameters, output);

	return E_NOTIMPL;
}

extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor **begin, scgms::TSignal_Descriptor **end) {
#ifndef NO_BUILD_DMMS
	*begin = const_cast<scgms::TSignal_Descriptor*>(signals_descriptors.data());
	*end = *begin + signals_descriptors.size();
	return S_OK;
#else
	return S_FALSE;
#endif
}
