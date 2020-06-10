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
#include "v1_boluses.h"
#include "basal_and_bolus.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <array>
#include <string>


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


const std::array<scgms::TModel_Descriptor, 2> model_descriptions = { { icarus_v1_boluses::desc, basal_and_bolus::desc } };


HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	*begin = const_cast<scgms::TModel_Descriptor*>(model_descriptions.data());
	*end = *begin + model_descriptions.size();
	return S_OK;
}


HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	if (!model_id) return E_INVALIDARG;

	if (*model_id == icarus_v1_boluses::model_id) 
		return Manufacture_Object<CV1_Boluses>(model, parameters, output);
	else if (*model_id == basal_and_bolus::model_id)
		return Manufacture_Object<CBasal_And_Bolus> (model, parameters, output);


	return E_NOTIMPL;
}