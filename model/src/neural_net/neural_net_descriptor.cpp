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


#include "neural_net_descriptor.h"

#include "../../../../common/utils/descriptor_utils.h"
#include "../../../../common/lang/dstrings.h"

namespace neural_net {
	size_t Level_To_Histogram_Index(const double level) {
		if (level >= High_Threshold) return Band_Count - 1;

		const double tmp = level - Low_Threshold;
		if (tmp < 0.0) return 0;

		return static_cast<size_t>(floor(tmp * Inv_Band_Size));
	}

	double Histogram_Index_To_Level(const size_t index) {
		if (index == 0) return Low_Threshold - Half_Band_Size;
		if (index >= Band_Count - 1) return High_Threshold + Half_Band_Size;

		return Low_Threshold + static_cast<double>(index - 1) * Band_Size + Half_Band_Size;
	}

	const scgms::TSignal_Descriptor sig_desc{ signal_Neural_Net_Prediction, dsNeural_Net_Prediction_Signal, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF1FBDFF, 0xFF1FBDFF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
}

namespace const_neural_net {
	std::vector<std::wstring> name_placeholder(2 * param_count);

	constexpr GUID calc_id = { 0xe34c6737, 0x1637, 0x48d1, { 0x9a, 0x53, 0xe8, 0xba, 0xd, 0xa, 0x5, 0x60 } };	// {E34C6737-1637-48D1-9A53-E8BA0D0A0560}	

	const TParameters lower_bound = Set_Double_First_Followers<TParameters>(30.0*scgms::One_Minute, -100.0, param_count);
	const TParameters default_parameters = Set_Double_First_Followers<TParameters>(30.0*scgms::One_Minute, 0.5, param_count);
	const TParameters upper_bound = Set_Double_First_Followers<TParameters>(30.0*scgms::One_Minute, 100.0, param_count);

	const std::array<const wchar_t *, param_count> calc_param_names =
		Name_Parameters_First_Followers<decltype(calc_param_names)>(dsDt, dsWeight, true, param_count, name_placeholder);
	const std::array<wchar_t *, param_count> calc_param_columns =
		Name_Parameters_First_Followers<decltype(calc_param_columns)>(rsDt_Column, rsWeight, false, param_count, name_placeholder);


	const std::array<scgms::NModel_Parameter_Value, param_count> calc_param_types =
		Set_Value_First_Followers<decltype(calc_param_types), scgms::NModel_Parameter_Value>(scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, param_count);

	const size_t signal_count = 1;
	const GUID signal_ids[signal_count] = { neural_net::signal_Neural_Net_Prediction };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_IG };


	scgms::TModel_Descriptor get_calc_desc() {
		const scgms::TModel_Descriptor result = {
			calc_id,
			scgms::NModel_Flags::Signal_Model,
			dsNeural_Net_Prediction_Model,
			rsNeural_Net_Prediction_Model,
			param_count,
			calc_param_types.data(),
			const_cast<const wchar_t**>(calc_param_names.data()),
			const_cast<const wchar_t**>(calc_param_columns.data()),
			lower_bound.vector.data(),
			default_parameters.vector.data(),
			upper_bound.vector.data(),
			signal_count,
			signal_ids,
			reference_signal_ids
		};

		return result;
	}
}

namespace reference_neural_net {

	const size_t filter_param_count = 1;
	const scgms::NParameter_Type filter_param_types[filter_param_count] = { scgms::NParameter_Type::ptRatTime };

	const wchar_t* filter_param_ui_names[filter_param_count] = { dsDt };
	const wchar_t* filter_param_config_names[filter_param_count] = { rsDt_Column };
	const wchar_t* filter_param_tooltips[filter_param_count] = { nullptr };

	const scgms::TFilter_Descriptor filter_desc = {
		filter_id,
		scgms::NFilter_Flags::None,
		dsReference_Neural_Net_Filter,
		filter_param_count,
		filter_param_types,
		filter_param_ui_names,
		filter_param_config_names,
		filter_param_tooltips
	};

}