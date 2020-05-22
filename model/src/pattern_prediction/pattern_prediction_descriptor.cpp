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

#include "../../../../common/utils/descriptor_utils.h"
#include "../../../../common/lang/dstrings.h"

#undef min

namespace pattern_prediction {
	scgms::TSignal_Descriptor get_sig_desc() {
		const scgms::TSignal_Descriptor sig_desc{ signal_Pattern_Prediction, dsPattern_Prediction_Signal, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFF5BD1F, 0xFFF5BD1F, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

		return sig_desc;
	}


	const size_t filter_param_count = 4;
	const scgms::NParameter_Type filter_param_types[filter_param_count] = { scgms::NParameter_Type::ptRatTime, scgms::NParameter_Type::ptWChar_Array, scgms::NParameter_Type::ptBool, scgms::NParameter_Type::ptBool };
	const wchar_t* filter_param_ui_names[filter_param_count] = { dsDt, dsParameters_File, dsUpdate_Parameters_File, dsDo_Not_Learn };
	const wchar_t* filter_param_config_names[filter_param_count] = { rsDt_Column, rsParameters_File, rsUpdate_Parameters_File, rsDo_Not_Learn };
	const wchar_t* filter_param_tooltips[filter_param_count] = { nullptr, nullptr, nullptr, nullptr };

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

}