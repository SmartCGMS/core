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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "executor.h"
#include "filters.h"
#include "device_event.h"

CFilter_Executor::CFilter_Executor(const GUID filter_id, std::recursive_mutex &communication_guard, scgms::IFilter *next_filter, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) :
	mCommunication_Guard(communication_guard),  mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data) {
	
	mFilter = create_filter_body(filter_id, next_filter);
}

void CFilter_Executor::Release_Filter() {
	if (mFilter) mFilter.reset();
}


HRESULT IfaceCalling CFilter_Executor::Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list* error_description) {

	if (!mFilter)
		return E_FAIL;

	HRESULT rc = mFilter->Configure(configuration, error_description);
	if ((rc == S_OK) && mOn_Filter_Created) {
		//at this point, we will call a callback function to perform any additional configuration of the filter we've just configured 
		rc = mOn_Filter_Created(mFilter.get(), mOn_Filter_Created_Data);
	}

	return rc;
}

HRESULT IfaceCalling CFilter_Executor::Execute(scgms::IDevice_Event *event) {
	//Simply acquire the lock and then call execute method of the filter
	std::lock_guard<std::recursive_mutex> guard{ mCommunication_Guard };
	
	return mFilter->Execute(event);
}


HRESULT IfaceCalling CFilter_Executor::QueryInterface(const GUID*  riid, void ** ppvObj) {
	return mFilter ? mFilter->QueryInterface(riid, ppvObj) : E_FAIL;
}

CTerminal_Filter::CTerminal_Filter(scgms::IFilter *custom_output) : mCustom_Output(custom_output) {

}

void CTerminal_Filter::Wait_For_Shutdown() {	
	std::unique_lock<std::mutex> guard{ mShutdown_Guard };
	mShutdown_Condition.wait(guard, [this]() {return mShutdown_Received; });
}


HRESULT IfaceCalling CTerminal_Filter::Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list* error_description) {
	return S_OK;
}

HRESULT IfaceCalling CTerminal_Filter::Execute(scgms::IDevice_Event *event) {
	
	if (!event) return E_INVALIDARG;

	scgms::TDevice_Event *raw_event;
	HRESULT rc = event->Raw(&raw_event);
	if (rc != S_OK) {
		event->Release();
		return rc;
	}

	if (raw_event->event_code == scgms::NDevice_Event_Code::Shut_Down) {
		mShutdown_Received = true;
		mShutdown_Condition.notify_all();
	}
		
	if (mCustom_Output) //if there's anybody interested in consuming the event
		return mCustom_Output->Execute(event);	

	event->Release();	//instead of us releasing it
	
	return S_OK; 
};


CCopying_Terminal_Filter::CCopying_Terminal_Filter(std::vector<CDevice_Event> &events, bool do_not_copy_info_events) : CTerminal_Filter(nullptr), mEvents(events), mDo_Not_Copy_Info_Events(do_not_copy_info_events) {

}

HRESULT IfaceCalling CCopying_Terminal_Filter::Execute(scgms::IDevice_Event *event) {
	scgms::TDevice_Event* raw_event;
		
	const HRESULT rc = event->Raw(&raw_event);
	if (Succeeded(rc)) {

		if (mDo_Not_Copy_Info_Events && (scgms::UDevice_Event_internal::major_type(raw_event->event_code) == scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info)) {
			return CTerminal_Filter::Execute(event);
		} else {
			CDevice_Event clone;
			clone.Initialize(raw_event);		


			mEvents.push_back(std::move(clone));
			return CTerminal_Filter::Execute(event);
		}
	}
	else
		return rc;
}
