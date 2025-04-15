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

#include <scgms/lang/dstrings.h>
#include <scgms/rtl/manufactory.h>
#include <scgms/rtl/UILib.h>
#include <scgms/utils/descriptor_utils.h>
#include <scgms/rtl/DeviceLib.h>
#include <scgms/rtl/FilterLib.h>

#include "basal_2_bolus.h"

#include <vector>

namespace basal_2_bolus {
	const size_t filter_param_count = 1;
	const scgms::NParameter_Type filter_param_types[filter_param_count] = { scgms::NParameter_Type::ptDouble_Array};
	const wchar_t* filter_param_ui_names[filter_param_count] = { dsParameters};
	const wchar_t* filter_param_config_names[filter_param_count] = { rsParameters };
	const wchar_t* filter_param_tooltips[filter_param_count] = { nullptr};

	const scgms::TFilter_Descriptor filter_desc = {
		filter_id,
		scgms::NFilter_Flags::Encapsulated_Model,
		dsBasal_2_Bolus,
		filter_param_count,
		filter_param_types,
		filter_param_ui_names,
		filter_param_config_names,
		filter_param_tooltips
	};


	const scgms::NModel_Parameter_Value model_types[model_param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime };
	const wchar_t* model_param_ui_names[model_param_count] = { dsMinimum_Volume, dsPPeriod };
	const wchar_t* model_param_config_names[model_param_count] = { rsMinimum, rsPeriod };

	const double lower_bound[model_param_count] = { 0.0, 0.0};	
	const double upper_bound[model_param_count] = { 100.0, 1.0 };

	const scgms::TModel_Descriptor model_desc = {
		filter_id,
		scgms::NModel_Flags::None,
		dsParameters,
		rsParameters,

		model_param_count,
		0,
		model_types,
		model_param_ui_names,
		model_param_config_names,

		lower_bound,
		default_parameters,
		upper_bound,

		0,
		&scgms::signal_Null,
		&scgms::signal_Null
	};
}

