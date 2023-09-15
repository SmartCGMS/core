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

#include "../../../common/iface/SimpleIface.h"
#include "../../../common/iface/FilterIface.h"


#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "filter_configuration_executor.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CSimple_SCGMS_Execution : public virtual scgms::IFilter, public virtual refcnt::CNotReferenced {
protected:
	TSCGMS_Execution_Callback mCallback;
	scgms::SFilter_Executor mExecutor;
	refcnt::Swstr_list mErrors;
public:
	CSimple_SCGMS_Execution(TSCGMS_Execution_Callback callback) : mCallback(callback) {
	}

	~CSimple_SCGMS_Execution() {
		if (mCallback && (mErrors->empty() != S_OK)) {
			TSCGMS_Event_Data err;
			memset(&err, 0, sizeof(err));

			err.device_time = std::numeric_limits<double>::quiet_NaN();
			err.event_code = static_cast<decltype(err.event_code)>(scgms::NDevice_Event_Code::Error);

			refcnt::wstr_container *wstr;
			while (mErrors->pop(&wstr) == S_OK) {
				const auto err_str = refcnt::WChar_Container_To_WString(wstr);
				err.str = const_cast<wchar_t*>(err_str.c_str());
				mCallback(&err);
				wstr->Release();
			}
		}

	}


	bool Execute_Configuration(const char* config, scgms::TOn_Filter_Created filterCreatedCallback = nullptr, const void* filterCreatedCallbackData = nullptr) {

		mErrors = refcnt::Swstr_list{};
		scgms::SPersistent_Filter_Chain_Configuration configuration{};

		if (configuration->Load_From_Memory(config, strlen(config), mErrors.get()) == S_OK) {
			scgms::IFilter_Executor *executor;
			if (execute_filter_configuration(configuration.get(), filterCreatedCallback, filterCreatedCallbackData, mCallback ? this : nullptr, &executor, mErrors.get()) == S_OK)
				mExecutor.reset(executor, [](scgms::IFilter_Executor* obj_to_release) { if (obj_to_release != nullptr) obj_to_release->Release(); });
		}

		return mExecutor.operator bool();
	}

	virtual HRESULT IfaceCalling Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list *error_description) override final {
		return E_NOTIMPL;
	}

	virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event) override final {
		TSCGMS_Event_Data simple_event;		
		scgms::TDevice_Event *raw_event;
		memset(&simple_event, 0, sizeof(simple_event));
		std::wstring info_str;

		if (event->Raw(&raw_event) == S_OK) {
			simple_event.event_code = static_cast<decltype(simple_event.event_code)>(raw_event->event_code);
			simple_event.device_id = raw_event->device_id;
			simple_event.signal_id = raw_event->signal_id;
			
			simple_event.device_time = raw_event->device_time;
			simple_event.logical_time = raw_event->logical_time;
			simple_event.segment_id = raw_event->segment_id;

			simple_event.parameters = nullptr;
			simple_event.count = 0;
			simple_event.str = nullptr;

			switch (scgms::UDevice_Event_internal::major_type(raw_event->event_code)) {
				case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::level:
					simple_event.level = raw_event->level;
					break;
				
				case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:
					{
						double *begin, *end;
						if (raw_event->parameters->get(&begin, &end) == S_OK) {
							simple_event.parameters = begin;
							simple_event.count = std::distance(begin, end);
						}
						
					}
					break;

				case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:
					{
						info_str = refcnt::WChar_Container_To_WString(raw_event->info);
						simple_event.str = const_cast<wchar_t*>(info_str.c_str());
					}
				break;

				default: break;
			}
		}

		const HRESULT rc = mCallback(&simple_event);

		event->Release();

		return rc;
	}

	HRESULT Inject_Event(scgms::UDevice_Event &event) {
		if (!event) return E_INVALIDARG;

		scgms::IDevice_Event *raw_event = event.get();
		event.release();
		return mExecutor->Execute(raw_event);
	}

	HRESULT Terminate(const BOOL wait_for_shutdown) {
		return mExecutor->Terminate(wait_for_shutdown);
	}

};

#pragma warning( pop )

DLL_EXPORT scgms_execution_t SimpleCalling Execute_SCGMS_Configuration(const char* config, TSCGMS_Execution_Callback callback, scgms::TOn_Filter_Created filterCreatedCallback) {

	std::unique_ptr<CSimple_SCGMS_Execution> result = std::make_unique<CSimple_SCGMS_Execution>(callback);

	if (result)
		if (!result->Execute_Configuration(config, filterCreatedCallback))
			result.reset();
	
	CSimple_SCGMS_Execution* raw_result = result.get();
	result.release();
	return raw_result;
}

DLL_EXPORT BOOL SimpleCalling Inject_SCGMS_Event(const scgms_execution_t execution, const TSCGMS_Event_Data *simple_event) {

	if (!execution)
		return FALSE;

	if (simple_event->event_code >= static_cast<std::underlying_type_t<scgms::NDevice_Event_Code>>(scgms::NDevice_Event_Code::count))
		return FALSE;

	scgms::UDevice_Event event_to_send{ static_cast<scgms::NDevice_Event_Code>(simple_event->event_code) };
	event_to_send.device_id() = simple_event->device_id;
	event_to_send.signal_id() = simple_event->signal_id;

	event_to_send.device_time() = simple_event->device_time;
	event_to_send.segment_id() = simple_event->segment_id;

	switch (scgms::UDevice_Event_internal::major_type(event_to_send.event_code())) {
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::level:
			event_to_send.level() = simple_event->level;
			break;

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:
			event_to_send.parameters->add(simple_event->parameters, simple_event->parameters + simple_event->count);
			break;

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:
			event_to_send.info.set(simple_event->str);	
			break;

		default:
			break;
	}

	CSimple_SCGMS_Execution* executor = static_cast<CSimple_SCGMS_Execution*>(execution);
	
	return Succeeded(executor->Inject_Event(event_to_send)) ? TRUE : FALSE;
}

DLL_EXPORT BOOL SimpleCalling Shutdown_SCGMS(const scgms_execution_t execution, BOOL wait_for_shutdown) {

	if (!execution)
		return FALSE;

	// explicitly take ownership back to release the executor from memory correctly
	// this "reverses" the release method call in Execute_SCGMS_Configuration function
	std::unique_ptr<CSimple_SCGMS_Execution> executor{ static_cast<CSimple_SCGMS_Execution*>(execution) };
	return Succeeded(executor->Terminate(wait_for_shutdown));
}
