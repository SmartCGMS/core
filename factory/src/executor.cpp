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

#include "executor.h"
#include "filters.h"

CFilter_Executor::CFilter_Executor(const GUID filter_id, refcnt::SReferenced<glucose::IFilter_Communicator> communicator, glucose::IFilter *next_filter, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) :
	mCommunicator(communicator),  mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data) {
	
	mFilter = create_filter(filter_id, next_filter);			
}


HRESULT IfaceCalling CFilter_Executor::Configure(glucose::IFilter_Configuration* configuration) {
	if (!mFilter) return E_FAIL;
	HRESULT rc = mFilter->Configure(configuration);
	if (rc == S_OK)
		//at this point, we will call a callback function to perform any additional configuration of the filter we've just configured 
		rc = mOn_Filter_Created(mFilter.get(), mOn_Filter_Created_Data);

	return rc;
}

HRESULT IfaceCalling CFilter_Executor::Execute(glucose::IDevice_Event *event) {
	//Simply acquire the lock and then call execute method of the filter
	glucose::CFilter_Communicator_Lock scoped_lock(mCommunicator);
	return mFilter->Execute(event);
}


HRESULT IfaceCalling CTerminal_Filter::Configure(glucose::IFilter_Configuration* configuration) {
	return S_OK;
}

HRESULT IfaceCalling CTerminal_Filter::Execute(glucose::IDevice_Event *event) {
	event->Release(); return S_OK; 
};