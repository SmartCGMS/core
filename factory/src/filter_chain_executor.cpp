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
 * Univerzitni 8
 * 301 00, Pilsen
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

#include "filter_chain_executor.h"
#include "device_event.h"

CFilter_Chain_Executor::CFilter_Chain_Executor(glucose::IEvent_Sender *output): mSender(output) {
	//
}


HRESULT CFilter_Chain_Executor::Configure(glucose::IFilter_Chain_Configuration *configuration, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) {

	glucose::IFilter_Executor *last_executor = static_cast<glucose::IFilter_Executor *>(this);

	glucose::IFilter_Configuration_Link **link_begin, **link_end;

	HRESULT rc = configuration->get(&link_begin, &link_end);
	if (rc != S_OK) return S_OK;

	//we have to create the filter executors from the last one
	glucose::IFilter_Configuration_Link *link = *link_end;
	try {
		do {
			//let's find out if this filter is synchronous or asynchronous
			GUID filter_id;
			rc = link->Get_Filter_Id(&filter_id);
			if (rc != S_OK) return rc;

			glucose::TFilter_Descriptor filter_desc = glucose::Null_Filter_Descriptor;
			bool async_filter = true;	//in the worst case, we can still execute the filter as asynchronous one
			if (glucose::get_filter_descriptor_by_id(filter_id, filter_desc)) async_filter = (filter_desc.flags & glucose::NFilter_Flags::Synchronous) != glucose::NFilter_Flags::Synchronous;

			std::unique_ptr<CExecutor> new_executor;
			if (async_filter)  new_executor = std::make_unique < CAsync_Filter_Executor>(filter_id, link, last_executor, on_filter_created, on_filter_created_data);
			else new_executor = std::make_unique<CSync_Filter_Executor>(filter_id, link, last_executor, on_filter_created, on_filter_created_data);

			last_executor = new_executor.get();
			mExecutors.insert(mExecutors.begin(), std::move(new_executor));

		} while (link != *link_begin);
	}
	catch (...) {
		return E_FAIL;
	}

	return S_OK;
}

HRESULT IfaceCalling CFilter_Chain_Executor::push_back(glucose::IDevice_Event *event) {
	if (mSender) return mSender->send(event);	//simply forward the event
		else {
			if (event) event->Release();		//or consume it
			return S_OK;					
		}
}


HRESULT IfaceCalling CFilter_Chain_Executor::send(glucose::IDevice_Event *event) {
	if (!event) return E_INVALIDARG;
	if (mExecutors.empty()) return S_FALSE;

	return mExecutors[0]->push_back(event);
}

CFilter_Chain_Executor::~CFilter_Chain_Executor() {
	Stop();
}

HRESULT IfaceCalling CFilter_Chain_Executor::Start() {
	for (size_t i = 0; i < mExecutors.size(); i++)	//no for each to ensure the order
		mExecutors[i]->start();

	return S_OK;
}

HRESULT IfaceCalling CFilter_Chain_Executor::Stop() {
	if (mExecutors.empty()) return S_FALSE;
		
	if (!SUCCEEDED(mExecutors[0]->push_back(static_cast<glucose::IDevice_Event*> (new CDevice_Event{ glucose::NDevice_Event_Code::Shut_Down }))))
		//if failed, let's abort 
		for (size_t i = 0; i < mExecutors.size(); i++) // no for each to ensure the order
			mExecutors[i]->abort();

	//wait for the filters as they terminate from the first to the last one
	for (size_t i = 0; i < mExecutors.size(); i++) // no for each to ensure the order
			mExecutors[i]->join();	

	return S_OK;
}

HRESULT IfaceCalling create_filter_chain_executor(glucose::IFilter_Chain_Configuration *configuration, glucose::IEvent_Sender *output,
												  glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
												  glucose::IFilter_Chain_Executor **executor) {

	std::unique_ptr<CFilter_Chain_Executor> raw_executor = std::make_unique<CFilter_Chain_Executor>(output);
	HRESULT rc = raw_executor->Configure(configuration, on_filter_created, on_filter_created_data);
	if (!SUCCEEDED(rc)) return rc;
	
	*executor = static_cast<glucose::IFilter_Chain_Executor*>(raw_executor.get());
	(*executor)->AddRef();
	raw_executor.release();

	return S_OK;
}


