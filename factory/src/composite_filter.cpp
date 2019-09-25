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

#include "composite_filter.h"
#include "device_event.h"
#include "filters.h"

CComposite_Filter::CComposite_Filter(glucose::IFilter_Communicator* communicator, glucose::IFilter *next_filter) : 
	mNext_Filter(next_filter), mCommunicator(refcnt::make_shared_reference_ext<glucose::SFilter_Communicator, glucose::IFilter_Communicator>(communicator, true)) {
	//
}


CComposite_Filter::~CComposite_Filter() {
}


HRESULT CComposite_Filter::Build_Filter_Chain(glucose::IFilter_Chain_Configuration *configuration, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) {
	if (!mExecutors.empty()) return E_ILLEGAL_METHOD_CALL;	//so far, we are able to configure the chain just once

	glucose::CFilter_Communicator_Lock communicator_lock(mCommunicator);
	glucose::IFilter *last_filter = mNext_Filter.get();
	
	glucose::IFilter_Configuration_Link **link_begin, **link_end;
	HRESULT rc = configuration->get(&link_begin, &link_end);
	if (rc != S_OK) return rc;

	//we have to create the filter executors from the last one
	glucose::IFilter_Configuration_Link *link = *(link_end-1);	
	try {
		do {
			//let's find out if this filter is synchronous or asynchronous
			GUID filter_id;
			rc = link->Get_Filter_Id(&filter_id);
			if (rc != S_OK) return rc;

			std::unique_ptr<CFilter_Executor> new_executor_raw = std::make_unique<CFilter_Executor>(filter_id, mCommunicator, last_filter, on_filter_created, on_filter_created_data);
			//try to configure the filter
			rc = new_executor_raw->Configure(link);
			if (rc != S_OK) return rc;

			//filter is configured, insert it into the chain
			last_filter = new_executor_raw.get();			
			mExecutors.insert(mExecutors.begin(), glucose::SFilter(new_executor_raw.get()));
			new_executor_raw.release();

		} while (link != *link_begin);
	}
	catch (...) {
		return E_FAIL;
	}

	return S_OK;
}

HRESULT IfaceCalling CComposite_Filter::Configure(glucose::IFilter_Configuration* configuration) {
	return E_ILLEGAL_METHOD_CALL;
}

HRESULT IfaceCalling CComposite_Filter::Execute(glucose::IDevice_Event *event) {
	if (!event) return E_INVALIDARG;
	if (mExecutors.empty()) return S_FALSE;	

	return mExecutors[0]->Execute(event);	
}

HRESULT IfaceCalling create_composite_filter(glucose::IFilter_Chain_Configuration *configuration,
											 glucose::IFilter_Communicator* communicator, glucose::IFilter *next_filter,
											 glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
											 glucose::IFilter **filter) {

	std::unique_ptr<CComposite_Filter> raw_filter = std::make_unique<CComposite_Filter>(communicator, next_filter);
	HRESULT rc = raw_filter->Build_Filter_Chain(configuration, on_filter_created, on_filter_created_data);
	if (!SUCCEEDED(rc)) return rc;
	
	*filter = static_cast<glucose::IFilter*>(raw_filter.get());
	(*filter)->AddRef();
	raw_filter.release();

	return S_OK;
}