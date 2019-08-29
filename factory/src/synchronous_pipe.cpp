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

#include "synchronous_pipe.h"

#include "../../../common/rtl/manufactory.h"

HRESULT IfaceCalling create_filter_synchronous_pipe(glucose::IFilter_Synchronous_Pipe **pipe) {
	return Manufacture_Object<CFilter_Synchronous_Pipe, glucose::IFilter_Synchronous_Pipe>(pipe);
}

CFilter_Synchronous_Pipe::CFilter_Synchronous_Pipe() noexcept
{
	mDeviceEvents = refcnt::Create_Container_shared<glucose::IDevice_Event*>(nullptr, nullptr);
	mQueue.set_capacity(mDefault_Capacity);
}

HRESULT CFilter_Synchronous_Pipe::send(glucose::IDevice_Event* event) {
	if (event == nullptr)
		return E_INVALIDARG;

	if (mShutting_Down_Send)
		return S_FALSE;

	glucose::TDevice_Event *raw_event;
	HRESULT rc = event->Raw(&raw_event);
	if (rc != S_OK)
		return rc;

	if (raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down)
		mShutting_Down_Send = true;

	std::unique_lock<std::mutex> lck(mInMutex);

	// add event to container; "event" is local pointer, so &event would be invalid outside the scope
	// but we use this pointer just here and in callee's Execute methods
	mDeviceEvents->add(&event, &event + 1);	//since now, event is nullptr as we do not own it anymore

	// iterate through all managed synchronous filters within one thread
	for (auto& filter : mFilters) {
		if (filter->Execute(mDeviceEvents.get()) != S_OK)
			return rc;
	}

	// move all events from container to pipe; preserve order of creation
	while (mDeviceEvents->pop(&event) == S_OK)
		mQueue.push(event);

	// clear the container, all device events were passed to output queue
	mDeviceEvents->set(nullptr, nullptr);

	return S_OK;
}

HRESULT CFilter_Synchronous_Pipe::receive(glucose::IDevice_Event** event)
{
	if (mShutting_Down_Receive)
		return S_FALSE;

	try {
		mQueue.pop(*event);

		glucose::TDevice_Event *raw_event;
		if ((*event)->Raw(&raw_event) == S_OK)	// should be OK since we call it in ::send
			mShutting_Down_Receive = (raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down);
	}
	catch (tbb::user_abort &) {
		mShutting_Down_Receive = true;
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CFilter_Synchronous_Pipe::abort()
{
	mShutting_Down_Send = mShutting_Down_Receive = true;
	return S_OK;
}

HRESULT CFilter_Synchronous_Pipe::add_filter(glucose::ISynchronous_Filter* synchronous_filter)
{
	mFilters.push_back(refcnt::make_shared_reference_ext<glucose::SSynchronous_Filter, glucose::ISynchronous_Filter>(synchronous_filter, true));
	return S_OK;
}
