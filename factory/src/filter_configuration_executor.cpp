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

#include "filter_configuration_executor.h"

#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/FilterLib.h"

#include "executor.h"
#include "composite_filter.h"
#include "device_event.h"

HRESULT CFilter_Configuration_Executor::Build_Filter_Chain(glucose::IFilter_Chain_Configuration *configuration, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) {
	return mComposite_Filter.Build_Filter_Chain(configuration, &mTerminal_Filter, on_filter_created, on_filter_created_data);
}


CFilter_Configuration_Executor::~CFilter_Configuration_Executor() {
	Terminate();
}

HRESULT IfaceCalling CFilter_Configuration_Executor::Execute(glucose::IDevice_Event *event) {	
	if (!event) return E_INVALIDARG;
	HRESULT rc = mComposite_Filter.Execute(event);
	if (rc != S_OK) event->Release();	//in the case of a failure, we consume the event anyway
	return rc;
}

HRESULT IfaceCalling CFilter_Configuration_Executor::Wait_For_Shutdown_and_Terminate() {
	if (mComposite_Filter.Empty()) return S_FALSE;
	mTerminal_Filter.Wait_For_Shutdown();
	mComposite_Filter.Clear();
	return S_OK;		
}

HRESULT IfaceCalling CFilter_Configuration_Executor::Terminate() {	
	return mComposite_Filter.Clear();
}

HRESULT IfaceCalling execute_filter_configuration(glucose::IFilter_Chain_Configuration *configuration, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data, glucose::IFilter_Executor **executor) {
	std::unique_ptr<CFilter_Configuration_Executor> raw_executor = std::make_unique<CFilter_Configuration_Executor>();
	//increase the reference just in a case that we would be released prematurely in the Build_Filter_Chain call
	*executor = static_cast<glucose::IFilter_Executor*>(raw_executor.get());
	(*executor)->AddRef();	

	HRESULT rc = raw_executor->Build_Filter_Chain(configuration, on_filter_created, on_filter_created_data);
	raw_executor.release();	//can release the unique pointer as it did its job and is needed no more

	if (!SUCCEEDED(rc)) {
		(*executor)->Release();
		return rc;
	}

	

	return S_OK;
}