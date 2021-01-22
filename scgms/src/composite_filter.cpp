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

#include "composite_filter.h"
#include "device_event.h"
#include "filters.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/string_utils.h" 
#include "../../../common/rtl/UILib.h"

#include <map>
#include <stdexcept>

CComposite_Filter::CComposite_Filter(std::recursive_mutex &communication_guard) : mCommunication_Guard(communication_guard) {
	//
}


HRESULT CComposite_Filter::Build_Filter_Chain(scgms::IFilter_Chain_Configuration *configuration, scgms::IFilter *next_filter, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data, refcnt::Swstr_list& error_description) {
	mRefuse_Execute = true;
	if (!mExecutors.empty())
		return E_ILLEGAL_METHOD_CALL;	//so far, we are able to configure the chain just once

	if (!configuration || !next_filter) return E_INVALIDARG;

	std::lock_guard<std::recursive_mutex> guard{ mCommunication_Guard };
	scgms::IFilter *last_filter = next_filter;
		
	scgms::IFilter_Configuration_Link **link_begin, **link_end;
	HRESULT rc = configuration->get(&link_begin, &link_end);
	if (rc != S_OK) {
		error_description.push(dsCannot_read_configuration);
		return rc;
	}

	//we have to create the filter executors from the last one
	try {
		//1st round - create the filters
		do {
			scgms::IFilter_Configuration_Link* &link = *(link_end-1);

			GUID filter_id;
			rc = link->Get_Filter_Id(&filter_id);
			if (rc != S_OK) {
				error_description.push(dsCannot_read_filter_id);
				mExecutors.clear();
				return rc;
			}

			std::unique_ptr<CFilter_Executor> new_executor = std::make_unique<CFilter_Executor>(filter_id, mCommunication_Guard, last_filter, on_filter_created, on_filter_created_data);
			//try to configure the filter 
			rc = new_executor->Configure(link, error_description.get());
			if (!Succeeded(rc)) {
				//describe such an event anyway just in the case the filter would not do so - hence we would at least know the configuration-failing filter
				std::wstring err_str{dsFailed_to_configure_filter};
				err_str += GUID_To_WString(filter_id);
				
				bool failed_to_resolve_descriptor = false;
				{//try to obtain filter's name
					scgms::TFilter_Descriptor desc = scgms::Null_Filter_Descriptor;
					if (scgms::get_filter_descriptor_by_id(filter_id, desc) ) {
						err_str += L" \"";
						err_str += desc.description;
						err_str += L'"';
					}
					else
						failed_to_resolve_descriptor = true;
				}
				error_description.push(err_str.c_str());

				if (failed_to_resolve_descriptor)
					describe_loaded_filters(error_description);

				if (!mExecutors.empty())
					mExecutors[0]->Execute(static_cast<scgms::IDevice_Event*> (new CDevice_Event{ scgms::NDevice_Event_Code::Shut_Down }));

				mExecutors.clear();
				return rc;
			}

			//filter is configured, insert it into the chain
			last_filter = new_executor.get();
			mExecutors.insert(mExecutors.begin(), std::move(new_executor));
			
			link_end--;
		} while (link_end != link_begin);

		//2nd round - gather information about the feedback receivers
		std::map<std::wstring, scgms::SFilter_Feedback_Receiver> feedback_map;
		for (auto &possible_receiver : mExecutors) {
			scgms::SFilter_Feedback_Receiver feedback_receiver;
			refcnt::Query_Interface<scgms::IFilter, scgms::IFilter_Feedback_Receiver>(possible_receiver.get(), scgms::IID_Filter_Feedback_Receiver, feedback_receiver);
			if (feedback_receiver) {
				wchar_t *name;
				if (feedback_receiver->Name(&name) == S_OK) {
					feedback_map[name] = feedback_receiver;
				}
			}
		}

		//3nd round - set the receivers to the senders
		//multiple senders can connect to a single receiver (so that we can have a single feedback filter)
		if (!feedback_map.empty())
			for (auto &possible_sender : mExecutors) {
				refcnt::SReferenced<scgms::IFilter_Feedback_Sender> feedback_sender;
				refcnt::Query_Interface<scgms::IFilter, scgms::IFilter_Feedback_Sender>(possible_sender.get(), scgms::IID_Filter_Feedback_Sender, feedback_sender);
				if (feedback_sender) {
					wchar_t *name;
					if (feedback_sender->Name(&name) == S_OK) {

						auto feedback_receiver = feedback_map.find(name);
						if (feedback_receiver != feedback_map.end()) {
							feedback_sender->Sink(feedback_receiver->second.get());
						}
						else {
							std::wstring err_str{ dsFeedback_sender_not_connected };
							err_str += name;
							//error_description.push(err_str.c_str()); - see the catch block
							throw std::runtime_error{ Narrow_WString(err_str ) };	//this is very likely severe error in the configuration, hence we stop it
						}
					}
				}
			}
	}
	catch (const std::exception & ex) {
		// specific handling for all exceptions extending std::exception, except
		// std::runtime_error which is handled explicitly
		mExecutors.clear();
		std::wstring error_desc = Widen_Char(ex.what());
		error_description.push(error_desc.c_str());
		return E_FAIL;
	}
	catch (...) {
		mExecutors.clear();
		return E_FAIL;
	}

	mRefuse_Execute = false;
	return S_OK;
}

HRESULT CComposite_Filter::Execute(scgms::IDevice_Event *event) {
	if (!event) return E_INVALIDARG;
	if (mExecutors.empty()) {
		event->Release();
		return S_FALSE;
	}

	std::lock_guard<std::recursive_mutex> lock_guard{ mCommunication_Guard };
	if (mRefuse_Execute) {
		event->Release();
		return E_ILLEGAL_METHOD_CALL;
	}

	return mExecutors[0]->Execute(event);	//and by this, we delegate event's release to the filters
}

HRESULT CComposite_Filter::Clear() {
		//obtain the communication guard/lock to ensure that no new communication will be accepted
		//via the execute method
	try {
		{
			std::lock_guard<std::recursive_mutex> guard{ mCommunication_Guard };
			mRefuse_Execute = true;
		}

		//once we refuse any communication from the Execute method, we can safely release the filters
		//assuming that they terminate any threads they have spawned
		for (size_t i = 0; i < mExecutors.size(); i++)
			mExecutors[i]->Release_Filter();
		mExecutors.clear();	//calls reset on all contained unique ptr's
	}
	catch (...) {
		return E_FAIL;
	}

	return S_OK;
}

bool CComposite_Filter::Empty() {
	return mExecutors.empty();
}
