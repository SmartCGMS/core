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

#include "pipe.h"

#include "../../../common/rtl/manufactory.h"

HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe) {
	return Manufacture_Object<CFilter_Pipe, glucose::IFilter_Pipe>(pipe);
}

CFilter_Pipe::CFilter_Pipe() noexcept {
	mQueue.set_capacity(mDefault_Capacity);
}

CFilter_Pipe::~CFilter_Pipe() {
	//just empty to call all the dctors correctly
}

HRESULT CFilter_Pipe::send(glucose::IDevice_Event* event) {
	if (event == nullptr)
		return E_INVALIDARG;

	if (mShutting_Down_Send)
		return S_FALSE;

	glucose::TDevice_Event *raw_event;
	HRESULT rc = event->Raw(&raw_event);
	if (rc != S_OK) return rc;
	
	if (raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down)
		mShutting_Down_Send = true;

	try {
		mQueue.push(event);
	}
	catch (tbb::user_abort &) {
		mShutting_Down_Send = true;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::receive(glucose::IDevice_Event** event) {

	if (mShutting_Down_Receive)
		return S_FALSE;

	try {
		mQueue.pop(*event);

		glucose::TDevice_Event *raw_event;
		if ((*event)->Raw(&raw_event) == S_OK)	//should be OK since we call it in ::send
			mShutting_Down_Receive = raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down;
	}

	catch (tbb::user_abort &) {
		mShutting_Down_Receive = true;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::abort() {
	//TODO: https://stackoverflow.com/questions/16961990/thread-building-blocks-concurrent-bounded-queue-how-do-i-close-it?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
	mShutting_Down_Send = mShutting_Down_Receive = true;
	mQueue.abort();
	return S_OK;
}
