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

#include "event_export.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/UILib.h"

#include <iostream>

CEvent_Export_Filter::CEvent_Export_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	
}

HRESULT IfaceCalling CEvent_Export_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<scgms::IEvent_Export_Filter_Inspection>(scgms::IID_Event_Export_Filter_Inspection, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

HRESULT IfaceCalling CEvent_Export_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list &errors) {
	return S_OK;
}

HRESULT IfaceCalling CEvent_Export_Filter::Do_Execute(scgms::UDevice_Event evt)
{
	HRESULT rc = S_OK;

	if (!mCallbacks.empty())
	{
		std::unique_lock<std::mutex> lck(mRegister_Mtx);

		scgms::IModel_Parameter_Vector* parameters = evt.is_parameters_event() ? evt.parameters.get() : nullptr;
		refcnt::wstr_container* info = evt.is_info_event() ? evt.info.get() : nullptr;

		for (auto& cb : mCallbacks)
		{
			rc = cb.second(evt.event_code(), &evt.device_id(), &evt.signal_id(), evt.device_time(), evt.logical_time(), evt.segment_id(), evt.level(), parameters, info);
			if (!Succeeded(rc))
				break;
		}

		// Shut_Down unregisters all callbacks
		if (evt.event_code() == scgms::NDevice_Event_Code::Shut_Down)
			mCallbacks.clear();
	}

	// if a callback has failed, do not propagate event further; this may indicate a critical error
	if (!Succeeded(rc))
		return rc;

	return Send(evt);
}

HRESULT IfaceCalling CEvent_Export_Filter::Register_Callback(const GUID* registered_device_id, scgms::TEvent_Export_Callback callback)
{
	std::unique_lock<std::mutex> lck(mRegister_Mtx);

	auto itr = mCallbacks.find(*registered_device_id);
	if (itr != mCallbacks.end())
	{
		// already registered / with the same callback address
		if (itr->second == callback)
			return S_FALSE;
		else // with different callback address
			return E_FAIL;
	}

	mCallbacks[*registered_device_id] = callback;
	return S_OK;
}

HRESULT IfaceCalling CEvent_Export_Filter::Unregister_Callback(const GUID* registered_device_id)
{
	std::unique_lock<std::mutex> lck(mRegister_Mtx);

	auto itr = mCallbacks.find(*registered_device_id);
	// not registered at this time
	if (itr == mCallbacks.end())
		return S_FALSE;

	mCallbacks.erase(itr);
	return S_OK;
}
