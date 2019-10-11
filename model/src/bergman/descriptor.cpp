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
 * Univerzitni 8
 * 301 00, Pilsen
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

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/descriptor_utils.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/UILib.h"


namespace bergman_model {

	/*
	const wchar_t *param_config_names[param_count] = {
		rsBergman_p1,
		rsBergman_p2,
		rsBergman_p3,
		rsBergman_p4,
		rsBergman_Vi,
		rsBergman_BW,
		rsBergman_VgDist,
		rsBergman_d1rate,
		rsBergman_d2rate,
		rsBergman_irate,
		rsBergman_Gb,
		rsBergman_Ib,
		rsBergman_G0,
		rsBergman_diff2_p,
		rsBergman_diff2_cg,
		rsBergman_diff2_c
	};
	*/

	constexpr size_t filter_param_count = 4;
	const wchar_t *filter_ui_names[filter_param_count] = {
		dsParameters,
		dsFeedback_Name,
		dsSynchronize_to_Signal,
		dsSynchronization_Signal
	};

	const wchar_t *filter_config_names[filter_param_count] = {
		rsParameters,
		rsFeedback_Name,
		rsSynchronize_to_Signal,
		rsSynchronization_Signal
	};

	const wchar_t *filter_tooltips[filter_param_count] = {
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	constexpr glucose::NParameter_Type filter_param_types[filter_param_count] = {
		glucose::NParameter_Type::ptDouble_Array,
		glucose::NParameter_Type::ptWChar_Array,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptSignal_Id
	};

	glucose::TFilter_Descriptor filter_descriptor = {
		filter_id,
		glucose::NFilter_Flags::None,
		dsBergman_Minimal_Model,
		filter_param_count,
		filter_param_types,
		filter_ui_names,
		filter_config_names,
		filter_tooltips		
	};


	const wchar_t *model_param_ui_names[model_param_count] = {
		dsBergman_p1,
		dsBergman_p2,
		dsBergman_p3,
		dsBergman_p4,
		dsBergman_Vi,
		dsBergman_BW,
		dsBergman_VgDist,
		dsBergman_d1rate,
		dsBergman_d2rate,
		dsBergman_irate,
		dsBergman_Gb,
		dsBergman_Ib,
		dsBergman_G0,
		dsBergman_diff2_p,
		dsBergman_diff2_cg,
		dsBergman_diff2_c
	};

	const glucose::NModel_Parameter_Value model_param_types[model_param_count] = {
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble
	};

	glucose::TModel_Descriptor model_descriptor = {
		model_id,
		dsBergman_Minimal_Model,
		nullptr,
		model_param_count,
		model_param_types,
		model_param_ui_names,
		nullptr,
		lower_bounds.vector,
		default_parameters.vector,
		upper_bounds.vector,

		//the data below should be corrected
		1,
		&glucose::signal_BG,
		&dsBlood,
		&glucose::signal_BG,
	};
}

static const std::array<glucose::TFilter_Descriptor, 1> bergman_filter_description = { bergman_model::filter_descriptor };


extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = &bergman_model::filter_descriptor;
	*end = *begin + 1;
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter *output, glucose::IFilter **filter) {
	return ENOENT;
}

extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end) {
	*begin = &bergman_model::model_descriptor;
	*end = *begin + 1;
	return S_OK;
}