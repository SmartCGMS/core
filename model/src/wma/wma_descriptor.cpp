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


#include "wma_descriptor.h"

#include "../../../../common/utils/descriptor_utils.h"
#include "../../../../common/lang/dstrings.h"

#include <vector>

#undef min

namespace wma {
	scgms::TSignal_Descriptor get_sig_desc() {
		const scgms::TSignal_Descriptor sig_desc{ signal_id, dsWeighted_Moving_Average, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFF5BD1F, 0xFF15BDFF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

		return sig_desc;
	}



	const size_t model_param_count = 13;
	const scgms::NModel_Parameter_Value filter_param_types[model_param_count] = { scgms::NModel_Parameter_Value::mptTime,
		scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble };
		
	const wchar_t* filter_param_ui_names[model_param_count] = { dsDt,
		dsWeight_0, dsWeight_5, dsWeight_10, dsWeight_15, dsWeight_20, dsWeight_25, dsWeight_30,
		dsWeight_35, dsWeight_40, dsWeight_45, dsWeight_50, dsWeight_55 };			

	const wchar_t* filter_param_config_names[model_param_count] = { rsDt_Column, 		
		rsWeight_0, rsWeight_5, rsWeight_10, rsWeight_15, rsWeight_20, rsWeight_25, rsWeight_30,
		rsWeight_35, rsWeight_40, rsWeight_45, rsWeight_50, rsWeight_55 };

	const wchar_t* filter_param_tooltips[model_param_count] = { nullptr, nullptr, nullptr, nullptr, nullptr,
																nullptr, nullptr, nullptr, nullptr,
																nullptr, nullptr, nullptr, nullptr };

	const double lower_bound[model_param_count] = {0.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0, };
	const double default_values[model_param_count] = { 20.0*scgms::One_Minute, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, };
	const double upper_bound[model_param_count] = { 60.0 * scgms::One_Minute, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0 };


	scgms::TModel_Descriptor get_model_desc() {
		const scgms::TModel_Descriptor model_desc = {
				model_id,
				scgms::NModel_Flags::Signal_Model,
				dsWeighted_Moving_Average,
				rsWeighted_Moving_Average,
								
				model_param_count,
				filter_param_types,
				filter_param_ui_names,
				filter_param_config_names,

				lower_bound,
				default_values,
				upper_bound,

				1,
				&signal_id,
				&scgms::signal_Null
		};

		return model_desc;
	}

}