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


#include "pattern_prediction_descriptor.h"
#include "pattern_descriptor_model_parameters.h"

#include "../../../../common/utils/descriptor_utils.h"
#include "../../../../common/lang/dstrings.h"

#include <cmath>

#undef min

namespace pattern_prediction {	

	scgms::TSignal_Descriptor get_sig_desc() {
		const scgms::TSignal_Descriptor sig_desc{ signal_Pattern_Prediction, dsPattern_Prediction_Signal, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFF5BD1F, 0xFFF5BD1F, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

		return sig_desc;
	}


	const size_t filter_param_count = 9;
	const scgms::NParameter_Type filter_param_types[filter_param_count] = { scgms::NParameter_Type::ptRatTime, scgms::NParameter_Type::ptWChar_Array, scgms::NParameter_Type::ptBool, scgms::NParameter_Type::ptBool, scgms::NParameter_Type::ptBool, scgms::NParameter_Type::ptDouble_Array, scgms::NParameter_Type::ptNull, scgms::NParameter_Type::ptInt64, scgms::NParameter_Type::ptWChar_Array };
	const wchar_t* filter_param_ui_names[filter_param_count] = { dsDt, dsParameters_File, dsDo_Not_Update_Parameters_File, dsDo_Not_Learn, dsUse_Config_parameters, dsParameters, dsExperimental, dsSliding_Window_Length, dsLearned_Data_Filename_Prefix };
	const wchar_t* filter_param_config_names[filter_param_count] = { rsDt_Column, rsParameters_File, rsDo_Not_Update_Parameters_File, rsDo_Not_Learn, rsUse_Config_parameters, rsParameters, nullptr, rsSliding_Window_Length, rsLearned_Data_Filename_Prefix };
	const wchar_t* filter_param_tooltips[filter_param_count] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	scgms::TFilter_Descriptor get_filter_desc() {
		const scgms::TFilter_Descriptor filter_desc = {
				filter_id,
				scgms::NFilter_Flags::None,
				dsPattern_Prediction_Model,
				filter_param_count,
				filter_param_types,
				filter_param_ui_names,
				filter_param_config_names,
				filter_param_tooltips
		};

		return filter_desc;
	}

	scgms::TModel_Descriptor get_model_desc() {
		const scgms::TModel_Descriptor model_desc = {
			pattern_prediction::filter_id,
			scgms::NModel_Flags::None,
			dsParameters,
			rsParameters,

			model_param_count,
			0,
			model_types,
			model_param_ui_names,
			model_param_config_names,

			lower_bound,
			default_values,
			upper_bound,

			0,
			& scgms::signal_Null,
			& scgms::signal_Null
		};

		return model_desc;
	}

	size_t Level_2_Band_Index(const double level) {
		if (level >= pattern_prediction::High_Threshold) return pattern_prediction::Band_Count - 1;

		const double tmp = level - pattern_prediction::Low_Threshold;
		if (tmp < 0.0) return 0;

		return static_cast<size_t>(std::round(tmp * pattern_prediction::Inv_Band_Size));
	}

	double Subclassed_Level(const double level) {
		//this routine won't get called often => no real need to optimize it

		const size_t band_idx = Level_2_Band_Index(level);

		double effective_level = level;
		double low_bound = pattern_prediction::Low_Threshold + static_cast<double>(band_idx) * pattern_prediction::Band_Size;
		double high_bound = low_bound + pattern_prediction::Band_Size;;

		//handle special cases
		if (band_idx == 0) {
			low_bound = 0.0;
			high_bound = pattern_prediction::Low_Threshold;
		}
		else if (level >= pattern_prediction::High_Threshold) {
			low_bound = pattern_prediction::High_Threshold;
			high_bound = 20.0; //makes no sense to consider higher values
			effective_level = std::min(high_bound, level);
		}

				
		const double subclass_size = (high_bound - low_bound) / static_cast<double>(Subclasses_Count);
		const double idx = (effective_level - low_bound) / subclass_size;
		return low_bound + std::round(idx) * subclass_size;
	}


}