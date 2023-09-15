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

#include "descriptor.h"
#include "v1_boluses.h"
#include "basal_and_bolus.h"
#include "rates_pack_boluses.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <array>
#include <string>

using namespace scgms::literals;

namespace icarus_v1_boluses {
	std::wstring ds_bt(const size_t n) { return dsBolus_Offset + std::to_wstring(n); };
	std::wstring ds_ba(const size_t n) { return dsBolus_Amount + std::to_wstring(n); };


	std::wstring rs_bt(const size_t n) { return rsBolus_Offset + std::to_wstring(n); };
	std::wstring rs_ba(const size_t n) { return rsBolus_Amount + std::to_wstring(n); };

	template <typename T>
	std::array<std::wstring, meal_count * 2> ar(T bt_f, T ba_f) {
		std::array<std::wstring, meal_count * 2> result;
		size_t idx = 0;
		for (size_t i = 1; i <= meal_count; i++) {
			result[idx++] = bt_f(i);
			result[idx++] = ba_f(i);
		}

		return result;
	}

	const std::array<std::wstring, meal_count * 2> ui_meals = ar(ds_bt, ds_ba);
	const std::array<std::wstring, meal_count * 2> config_meals = ar(rs_bt, rs_ba);

	const wchar_t* ui(const size_t i) { return ui_meals[i].c_str(); }
	const wchar_t* cfg(const size_t i) { return config_meals[i].c_str(); }

	const wchar_t* param_names[param_count] = { dsBasal_Insulin_Rate, ui(0), ui(1), ui(2), ui(3), ui(4), ui(5), ui(6), ui(7), ui(8), ui(9), ui(10), ui(11), ui(12), ui(13), ui(14), ui(15)};
	const wchar_t* param_columns[param_count] = { rsBasal_Insulin_Rate, cfg(0), cfg(1), cfg(2), cfg(3), cfg(4), cfg(5), cfg(6), cfg(7), cfg(8), cfg(9), cfg(10), cfg(11), cfg(12), cfg(13), cfg(14), cfg(15) };
	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble,
											scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, 
											scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, 
											scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, 
											scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, 
											scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, 
											scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, 
											scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble };

	const double experiment_time = 601.0 * 5.0 * scgms::One_Minute;
	const double meal_period = 6.095232 * scgms::One_Hour;

	const double lower_bound[param_count] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	const double upper_bound[param_count] = { 10.0, experiment_time, 100.0,  experiment_time, 100.0, experiment_time, 100.0, experiment_time, 100.0, 
											experiment_time, 100.0, experiment_time, 100.0, experiment_time, 100.0, experiment_time, 100.0 };

	const double default_parameters[param_count] = { 1.5, 1.0* meal_period, 10.0,  2.0 * meal_period, 10.0, 3.0 * meal_period, 10.0, 4.0 * meal_period, 10.0, 5.0 * meal_period, 10.0, 6.0 * meal_period, 10.0, 7.0 * meal_period, 10.0, 8.0 * meal_period, 10.0 };

	const scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsIcarus_v1_AI_Boluses,
		rsIcarus_v1_AI_Boluses,
		param_count,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		0,
		nullptr,
		nullptr
	};
}

namespace basal_and_bolus {
	const wchar_t* param_names[param_count] = { dsBasal_Insulin_Rate, dsCarb_To_Insulin_Ratio };
	const wchar_t* param_columns[param_count] = { rsBasal_Insulin_Rate, rsCarb_To_Insulin_Ratio };
	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble };

	const double lower_bound[param_count] = { 0.0, 0.0 };
	const double upper_bound[param_count] = { 10.0, 10.0 };

	const double default_parameters[param_count] = { 2.0, 1.0/20.20 };

	const scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsIcarus_Basal_And_Bolus,
		rsIcarus_Basal_And_Bolus,
		param_count,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		0,
		nullptr,
		nullptr
	};
}


namespace rates_pack_boluses {


	const double experiment_time = 601.0 * 5.0 * scgms::One_Minute;
	const double meal_period = experiment_time / static_cast<double>(bolus_max_count);
	const double rate_period = experiment_time / static_cast<double>(basal_change_max_count);

	const wchar_t* param_names[param_count] = { dsBasal_Insulin_Rate, dsSuspend_Threshold, dsLGS_Suspend_Duration, L"Bolus offset 0", L"Bolus 0", L"Bolus offset 1", L"Bolus 1", L"Bolus offset 2", L"Bolus 2", L"Bolus offset 3", L"Bolus 3", L"Bolus offset 4", L"Bolus 4", L"Bolus offset 5", L"Bolus 5", L"Bolus offset 6", L"Bolus 6", L"Bolus offset 7", L"Bolus 7", L"Basal rate offset 0", L"Basal rate 0", L"Basal rate offset 1", L"Basal rate 1", L"Basal rate offset 2", L"Basal rate 2", L"Basal rate offset 3", L"Basal rate 3", L"Basal rate offset 4", L"Basal rate 4", L"Basal rate offset 5", L"Basal rate 5", L"Basal rate offset 6", L"Basal rate 6", L"Basal rate offset 7", L"Basal rate 7", L"Basal rate offset 8", L"Basal rate 8", L"Basal rate offset 9", L"Basal rate 9", L"Basal rate offset 10", L"Basal rate 10", L"Basal rate offset 11", L"Basal rate 11", L"Basal rate offset 12", L"Basal rate 12", L"Basal rate offset 13", L"Basal rate 13", L"Basal rate offset 14", L"Basal rate 14", L"Basal rate offset 15", L"Basal rate 15", L"Basal rate offset 16", L"Basal rate 16", L"Basal rate offset 17", L"Basal rate 17", L"Basal rate offset 18", L"Basal rate 18", L"Basal rate offset 19", L"Basal rate 19", L"Basal rate offset 20", L"Basal rate 20", L"Basal rate offset 21", L"Basal rate 21", L"Basal rate offset 22", L"Basal rate 22", L"Basal rate offset 23", L"Basal rate 23", L"Basal rate offset 24", L"Basal rate 24", L"Basal rate offset 25", L"Basal rate 25", L"Basal rate offset 26", L"Basal rate 26", L"Basal rate offset 27", L"Basal rate 27", L"Basal rate offset 28", L"Basal rate 28", L"Basal rate offset 29", L"Basal rate 29", L"Basal rate offset 30", L"Basal rate 30", L"Basal rate offset 31", L"Basal rate 31", L"Basal rate offset 32", L"Basal rate 32", L"Basal rate offset 33", L"Basal rate 33", L"Basal rate offset 34", L"Basal rate 34", L"Basal rate offset 35", L"Basal rate 35", L"Basal rate offset 36", L"Basal rate 36", L"Basal rate offset 37", L"Basal rate 37", L"Basal rate offset 38", L"Basal rate 38", L"Basal rate offset 39", L"Basal rate 39" };
	const wchar_t* config_names[param_count] = { rsBasal_Insulin_Rate, rsSuspend_Threshold, rsLGS_Suspend_Duration, L"Bolus_Offset_0", L"Bolus_0", L"Bolus_Offset_1", L"Bolus_1", L"Bolus_Offset_2", L"Bolus_2", L"Bolus_Offset_3", L"Bolus_3", L"Bolus_Offset_4", L"Bolus_4", L"Bolus_Offset_5", L"Bolus_5", L"Bolus_Offset_6", L"Bolus_6", L"Bolus_Offset_7", L"Bolus_7", L"Basal_Rate_Offset 0", L"Basal_Rate_0", L"Basal_Rate_Offset 1", L"Basal_Rate_1", L"Basal_Rate_Offset 2", L"Basal_Rate_2", L"Basal_Rate_Offset 3", L"Basal_Rate_3", L"Basal_Rate_Offset 4", L"Basal_Rate_4", L"Basal_Rate_Offset 5", L"Basal_Rate_5", L"Basal_Rate_Offset 6", L"Basal_Rate_6", L"Basal_Rate_Offset 7", L"Basal_Rate_7", L"Basal_Rate_Offset 8", L"Basal_Rate_8", L"Basal_Rate_Offset 9", L"Basal_Rate_9", L"Basal_Rate_Offset 10", L"Basal_Rate_10", L"Basal_Rate_Offset 11", L"Basal_Rate_11", L"Basal_Rate_Offset 12", L"Basal_Rate_12", L"Basal_Rate_Offset 13", L"Basal_Rate_13", L"Basal_Rate_Offset 14", L"Basal_Rate_14", L"Basal_Rate_Offset 15", L"Basal_Rate_15", L"Basal_Rate_Offset 16", L"Basal_Rate_16", L"Basal_Rate_Offset 17", L"Basal_Rate_17", L"Basal_Rate_Offset 18", L"Basal_Rate_18", L"Basal_Rate_Offset 19", L"Basal_Rate_19", L"Basal_Rate_Offset 20", L"Basal_Rate_20", L"Basal_Rate_Offset 21", L"Basal_Rate_21", L"Basal_Rate_Offset 22", L"Basal_Rate_22", L"Basal_Rate_Offset 23", L"Basal_Rate_23", L"Basal_Rate_Offset 24", L"Basal_Rate_24", L"Basal_Rate_Offset 25", L"Basal_Rate_25", L"Basal_Rate_Offset 26", L"Basal_Rate_26", L"Basal_Rate_Offset 27", L"Basal_Rate_27", L"Basal_Rate_Offset 28", L"Basal_Rate_28", L"Basal_Rate_Offset 29", L"Basal_Rate_29", L"Basal_Rate_Offset 30", L"Basal_Rate_30", L"Basal_Rate_Offset 31", L"Basal_Rate_31", L"Basal_Rate_Offset 32", L"Basal_Rate_32", L"Basal_Rate_Offset 33", L"Basal_Rate_33", L"Basal_Rate_Offset 34", L"Basal_Rate_34", L"Basal_Rate_Offset 35", L"Basal_Rate_35", L"Basal_Rate_Offset 36", L"Basal_Rate_36", L"Basal_Rate_Offset 37", L"Basal_Rate_37", L"Basal_Rate_Offset 38", L"Basal_Rate_38", L"Basal_Rate_Offset 39", L"Basal_Rate_39" };
	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble };
	const double lower_bound[param_count] = { 0.0, 0.0, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Bolus_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0, Minimum_Rate_Change_Delay, 0.0 };
	const double default_parameters[param_count] = { 1.5, 3.0, 30_min, 0.0 * meal_period, 10.0, 1.0 * meal_period, 10.0, 2.0 * meal_period, 10.0, 3.0 * meal_period, 10.0, 4.0 * meal_period, 10.0, 5.0 * meal_period, 10.0, 6.0 * meal_period, 10.0, 7.0 * meal_period, 10.0, 0.0 * rate_period, 10.0, 1.0 * rate_period, 10.0, 2.0 * rate_period, 10.0, 3.0 * rate_period, 10.0, 4.0 * rate_period, 10.0, 5.0 * rate_period, 10.0, 6.0 * rate_period, 10.0, 7.0 * rate_period, 10.0, 8.0 * rate_period, 10.0, 9.0 * rate_period, 10.0, 10.0 * rate_period, 10.0, 11.0 * rate_period, 10.0, 12.0 * rate_period, 10.0, 13.0 * rate_period, 10.0, 14.0 * rate_period, 10.0, 15.0 * rate_period, 10.0, 16.0 * rate_period, 10.0, 17.0 * rate_period, 10.0, 18.0 * rate_period, 10.0, 19.0 * rate_period, 10.0, 20.0 * rate_period, 10.0, 21.0 * rate_period, 10.0, 22.0 * rate_period, 10.0, 23.0 * rate_period, 10.0, 24.0 * rate_period, 10.0, 25.0 * rate_period, 10.0, 26.0 * rate_period, 10.0, 27.0 * rate_period, 10.0, 28.0 * rate_period, 10.0, 29.0 * rate_period, 10.0, 30.0 * rate_period, 10.0, 31.0 * rate_period, 10.0, 32.0 * rate_period, 10.0, 33.0 * rate_period, 10.0, 34.0 * rate_period, 10.0, 35.0 * rate_period, 10.0, 36.0 * rate_period, 10.0, 37.0 * rate_period, 10.0, 38.0 * rate_period, 10.0, 39.0 * rate_period, 10.0 };
	const double upper_bound[param_count] = { 10.0, 5.0, 2_hr, experiment_time, 25.0, experiment_time, 25.0, experiment_time, 25.0, experiment_time, 25.0, experiment_time, 25.0, experiment_time, 25.0, experiment_time, 25.0, experiment_time, 25.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0, experiment_time, 12.0 };


	const scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"Icarus Rates Pack & Boluses & LGS",
		L"Icarus_Rates_Pack_Boluses",
		param_count,
		param_count,
		param_types,
		param_names,
		config_names,
		lower_bound,
		default_parameters,
		upper_bound,
		0,
		nullptr,
		nullptr
	};


	/*
#include <iostream>
#include <sstream>

int main() {
	constexpr size_t bolus_max_count = 8;
	constexpr size_t basal_change_max_count = 40;

	std::stringstream ui_names; ui_names << "const wchar_t* param_names[param_count] = { dsBasal_Insulin_Rate, dsSuspend_Threshold, dsLGS_Suspend_Duration";
	std::stringstream config_names; config_names << "const wchar_t* config_names[param_count] = { rsBasal_Insulin_Rate, rsSuspend_Threshold, rsLGS_Suspend_Duration";
	std::stringstream param_types; param_types << "const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime";

	std::stringstream lower; lower << "const double lower_bound[param_count] = {0.0, 0.0, 0.0";
	std::stringstream upper; upper << "const double upper_bound[param_count] = { 10.0, 5.0, 2_hr";
	std::stringstream def_params; def_params << "const double default_parameters[param_count] = { 1.5, 3.0, 30_min";


	for (size_t i=0; i<bolus_max_count; i++) {
		ui_names << ", L\"Bolus offset " << i << "\", L\"Bolus " << i << '"';
		config_names << ", L\"Bolus_Offset_" << i << "\", L\"Bolus_" << i << '"';
		param_types << ", scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble";


		lower << ", Minimum_Bolus_Delay, 0.0";
		def_params << ", " << i <<  ".0*meal_period, 10.0";
		upper << ", experiment_time, 25.0";
	}

	for (size_t i=0; i<basal_change_max_count; i++) {
		ui_names << ", L\"Basal rate offset " << i << "\", L\"Basal rate "<< i << '"';
		config_names << ", L\"Basal_Rate_Offset " << i << "\", L\"Basal_Rate_" << i << '"';
		param_types << ", scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble";

		lower << ", Minimum_Rate_Change_Delay, 0.0";
		def_params << ", " << i <<  ".0*rate_period, 10.0";
		upper << ", experiment_time, 12.0";
	}


	ui_names << "};";
	config_names << "};";
	param_types << "};";

	lower << "};";
	def_params << "};";
	upper << "};";


	std::cout << ui_names.str() << std::endl;
	std::cout << config_names.str() << std::endl;
	std::cout << param_types.str() << std::endl;

	std::cout << lower.str() << std::endl;
	std::cout << def_params.str() << std::endl;
	std::cout << upper.str() << std::endl;

	return 0;
}
	
	*/
}

const std::array<scgms::TModel_Descriptor, 3> model_descriptions = { { icarus_v1_boluses::desc, basal_and_bolus::desc,  rates_pack_boluses::desc} };


DLL_EXPORT HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	*begin = const_cast<scgms::TModel_Descriptor*>(model_descriptions.data());
	*end = *begin + model_descriptions.size();
	return S_OK;
}


DLL_EXPORT HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	if (!model_id) return E_INVALIDARG;

	if (*model_id == icarus_v1_boluses::model_id) 
		return Manufacture_Object<CV1_Boluses>(model, parameters, output);
	else if (*model_id == basal_and_bolus::model_id)
		return Manufacture_Object<CBasal_And_Bolus> (model, parameters, output);
	else if (*model_id == rates_pack_boluses::model_id)
		return Manufacture_Object<CRates_Pack_Boluses>(model, parameters, output);


	return E_NOTIMPL;
}