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

#include "native_script.h"

CNative_Script::CNative_Script(scgms::IFilter* output) : CBase_Filter(output) {

}

HRESULT CNative_Script::Do_Execute(scgms::UDevice_Event event) {
	HRESULT rc = E_UNEXPECTED;
	
	//Are we interested in this event?
	GUID signal_id = event.signal_id();

	bool desired_event = event.is_level_event();
	size_t sig_index = std::numeric_limits<size_t>::max();
	if (!mSync_To_Any) {
		const auto sig_iter = mSignal_To_Ordinal.find(signal_id);
		desired_event = sig_iter != mSignal_To_Ordinal.end();
		if (desired_event)
			sig_index = std::distance(mSignal_To_Ordinal.begin(), sig_iter);
	}


	if (desired_event) {
		//do we already have this segment?
		uint64_t segment_id = event.segment_id();
		auto seg_iter = mSegments.find(segment_id);
		if (seg_iter == mSegments.end()) {
			//not yet, we have to insert it
		}

	} else {
		//on segment stop, remove the segment from memory

		rc = mOutput.Send(event);
	}


	return rc;
}

HRESULT CNative_Script::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mSignal_Ids[0] = configuration.Read_GUID(rsSynchronization_Signal);
	if ((mSignal_Ids[0] == Invalid_GUID) || (mSignal_Ids[0] == scgms::signal_Null)) {
		error_description.push(L"Synchronization signal cannot be invalid or null id.");
		return E_INVALIDARG;
	}

	mSignal_To_Ordinal[mSignal_Ids[0]] = 0;
	mSync_To_Any = mSignal_Ids[0] == scgms::signal_All;

	for (size_t i = 1; i < mSignal_Ids.size(); i++) {
		const std::wstring res_name = native::rsRequired_Signal_Prefix + std::to_wstring(i + 1);
		const GUID sig_id = configuration.Read_GUID(res_name.c_str());
		mSignal_Ids[i] = sig_id;
		mSignal_To_Ordinal[sig_id] = i;
	}

	return S_OK;
}

