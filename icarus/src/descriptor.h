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

#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"
#include "../../../common/rtl/ModelsLib.h"


namespace icarus_v1_boluses {
	constexpr GUID model_id = { 0xf9d961a5, 0x4767, 0x41f6, { 0x89, 0x5f, 0x2a, 0xe6, 0xf4, 0xf4, 0x56, 0x4c } };	// {F9D961A5-4767-41F6-895F-2AE6F4F4564C}


	constexpr size_t meal_count = 8;
	constexpr size_t param_count = 1 + 2 * meal_count;	//basal rate + time & bolus for meals

	struct TMeal_Bolus {
		double offset;
		double bolus;
	};

	struct TParameters {
		union {
			struct {
				double basal_rate;
				TMeal_Bolus bolus[meal_count];

			};
			double vector[param_count];
		};
	};

	extern const double default_parameters[param_count];
}


namespace basal_and_bolus {
	constexpr GUID model_id = { 0x3498d722, 0x701f, 0x4745, { 0x9f, 0xa8, 0x69, 0x1d, 0xc0, 0xaf, 0xf8, 0xdf } }; // {3498D722-701F-4745-9FA8-691DC0AFF8DF}

	constexpr size_t param_count = 2;

	struct TParameters {
		union {
			struct {
				double basal_rate;
				double carb_to_insulin;	//carbs to insulin eliminates check for a division by zero and setting minimum value!
			};
			double vector[param_count];
		};
	};

	extern const double default_parameters[param_count];
}



namespace rates_pack_boluses {
	constexpr GUID model_id = { 0xa682f21e, 0x9f52, 0x41e6, { 0x90, 0x52, 0x23, 0xce, 0x44, 0xc7, 0x53, 0x35 } }; // {A682F21E-9F52-41E6-9052-23CE44C75335}

	struct TInsulin_Setting {
		double offset;
		double value;
	};

	constexpr size_t Insulin_Setting_Count = sizeof(TInsulin_Setting) / sizeof(double);

	constexpr size_t bolus_max_count = 8;
	constexpr size_t basal_change_max_count = 40;		//how many times we can change the basal rate settings
	constexpr size_t param_count = 3 + Insulin_Setting_Count * (bolus_max_count + basal_change_max_count);	//basal rate + time & bolus for meals


	struct TParameters {
		union {
			struct {
				double initial_basal_rate;
				double lgs_threshold;		//on which glucose level to switch insulin rate off
				double lgs_timeout;
				TInsulin_Setting boluses[bolus_max_count];
				TInsulin_Setting rates[basal_change_max_count];

			};
			double vector[param_count];
		};
	};

	extern const double default_parameters[param_count];
}



extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model);
