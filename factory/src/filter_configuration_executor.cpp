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

CFilter_Configuration_Executor::CFilter_Configuration_Executor(glucose::IFilter_Chain_Configuration *configuration, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) {
	mTerminal_Filter.Set_Communicator(&mCommunicator);

	glucose::IFilter *raw_composite;
	if (create_composite_filter(configuration, &mCommunicator, &mTerminal_Filter, on_filter_created, on_filter_created_data, &raw_composite) != S_OK) throw std::runtime_error{"Cannot create the filter chain!"};
	mComposite_Filter = raw_composite;
	//from now on, the asynchronous filters are already running	
}


CFilter_Configuration_Executor::~CFilter_Configuration_Executor() {
	Terminate();
}

HRESULT IfaceCalling CFilter_Configuration_Executor::Execute(glucose::IDevice_Event *event) {	
	if (!mComposite_Filter) return S_FALSE;

	return mComposite_Filter->Execute(event);
}

HRESULT IfaceCalling CFilter_Configuration_Executor::Wait_For_Shutdown_and_Terminate() {
	mTerminal_Filter.Wait_For_Shutdown();
	return Terminate();		
}

HRESULT IfaceCalling CFilter_Configuration_Executor::Terminate() {
	if (!mComposite_Filter) return E_ILLEGAL_STATE_CHANGE;

	glucose::CFilter_Communicator_Lock scoped_lock(&mCommunicator);

	//temporarily, we add more reference to the raw composite filter so that we can destroy the managing shared_ptr safely
	//but without destroying the managed object -> hence we can check whether we are really the ones who stops the executor
	glucose::IFilter *raw_composite = mComposite_Filter.get();
	raw_composite->AddRef();
	mComposite_Filter = nullptr;
	return raw_composite->Release() == 0 ? S_OK : S_FALSE;
}

HRESULT IfaceCalling execute_filter_configuration(glucose::IFilter_Chain_Configuration *configuration, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data, glucose::IFilter_Executor **executor) {
	return Manufacture_Object<CFilter_Configuration_Executor, glucose::IFilter_Executor>(executor, configuration, on_filter_created, on_filter_created_data);
}