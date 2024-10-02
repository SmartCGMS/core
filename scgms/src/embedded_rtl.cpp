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

#ifdef __wasm__

	#include "filters.h"
	#include "device_event.h"
	#include "filter_parameter.h"
	#include "configuration_link.h"
	#include "persistent_chain_configuration.h"
	#include "filter_configuration_executor.h"

	#include <scgms/rtl/FilterLib.h>

	namespace scgms {

		IDevice_Event* Create_Event(const NDevice_Event_Code code) {

			IDevice_Event* result = allocate_device_event(code);

			assert(result != nullptr);
			return result;
		}

		UDevice_Event::UDevice_Event(const NDevice_Event_Code code) noexcept : UDevice_Event(Create_Event(code)) {
		}

		SFilter_Parameter SFilter_Configuration_Link::Add_Parameter(const scgms::NParameter_Type type, const wchar_t* conf_name) {
			SFilter_Parameter result;
			scgms::IFilter_Parameter* parameter;
			if (Manufacture_Object<CFilter_Parameter, scgms::IFilter_Parameter>(&parameter, type, conf_name) == S_OK) {
				if (get()->add(&parameter, &parameter + 1) == S_OK) {
					result = refcnt::make_shared_reference_ext<SFilter_Parameter, IFilter_Parameter>(parameter, false);
				}
			}

			return result;
		}

		scgms::SFilter_Parameter scgms::internal::Create_Filter_Parameter(const scgms::NParameter_Type type, const wchar_t* config_name) {
			scgms::SFilter_Parameter result;
			scgms::IFilter_Parameter* new_parameter;
			if (Manufacture_Object<CFilter_Parameter, scgms::IFilter_Parameter>(&new_parameter, type, config_name) == S_OK) {			
				result = refcnt::make_shared_reference_ext<scgms::SFilter_Parameter, scgms::IFilter_Parameter>(new_parameter, false);
			}
			return result;
		}

		scgms::SFilter_Configuration_Link internal::Create_Configuration_Link(const GUID& id) {
			scgms::SFilter_Configuration_Link result;
			scgms::IFilter_Configuration_Link* link;
			if (Manufacture_Object<CFilter_Configuration_Link>(&link, id)  == S_OK) {
				result = refcnt::make_shared_reference_ext<scgms::SFilter_Configuration_Link, scgms::IFilter_Configuration_Link>(link, false);
			}

			return result;
		}

		SPersistent_Filter_Chain_Configuration::SPersistent_Filter_Chain_Configuration() {
			IPersistent_Filter_Chain_Configuration* configuration;
			if (Manufacture_Object<CPersistent_Chain_Configuration, scgms::IPersistent_Filter_Chain_Configuration>(&configuration) == S_OK) {
				reset(configuration, [](IPersistent_Filter_Chain_Configuration* obj_to_release) {
					if (obj_to_release != nullptr) {
						obj_to_release->Release();
					}
				});
			}
		}

		SFilter_Executor::SFilter_Executor(refcnt::SReferenced<scgms::IFilter_Chain_Configuration> configuration, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data, refcnt::Swstr_list error_description, scgms::IFilter* output) {
			scgms::IFilter_Executor* executor;
			const HRESULT rc = execute_filter_configuration(configuration.get(), on_filter_created, on_filter_created_data, output, &executor, error_description.get());
			if (Succeeded(rc)) {
				reset(executor, [](scgms::IFilter_Executor* obj_to_release) { if (obj_to_release != nullptr) obj_to_release->Release(); });
			}
		}

		std::vector<TFilter_Descriptor> get_filter_descriptor_list() {
			std::vector<TFilter_Descriptor> result;
			TFilter_Descriptor* desc_begin, * desc_end;

			if (get_filter_descriptors(&desc_begin, &desc_end) == S_OK) {
				std::copy(desc_begin, desc_end, std::back_inserter(result));
			}

			return result;
		}

		bool get_filter_descriptor_by_id(const GUID& id, TFilter_Descriptor& desc) {
			TFilter_Descriptor* desc_begin, * desc_end;

			bool result = get_filter_descriptors(&desc_begin, &desc_end) == S_OK;
			if (result) {
				result = false;	//we have to find the filter yet
				for (auto iter = desc_begin; iter != desc_end; iter++) {
					if (iter->id == id) {
						//desc = *iter;							assign const won't work with const members and custom operator= will result into undefined behavior as it has const members (so it does not have to be const itself)
						memcpy(&desc, iter, sizeof(decltype(desc)));	//=> memcpy https://stackoverflow.com/questions/9218454/struct-with-const-member
						result = true;
						break;
					}
				}
			}

			return result;
		}
	}

#endif