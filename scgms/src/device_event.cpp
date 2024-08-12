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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "device_event.h"

#include <scgms/rtl/rattime.h>
#include <scgms/rtl/manufactory.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/DeviceLib.h>
#include <scgms/utils/DebugHelper.h>

#include <atomic>
#include <stdexcept>


#include <array>
#include <atomic>
#include <tuple>


constexpr size_t Event_Pool_Size = 100*1024;

class CEvent_Pool {
	protected:
		std::array<CDevice_Event, Event_Pool_Size> mEvents;
		std::array<std::atomic<bool>, Event_Pool_Size > mAllocated_Flags{ false };
		std::atomic<size_t> mRecent_Allocated_Event_Idx{ Event_Pool_Size - 1 };

	public:
		CEvent_Pool() {
			for (size_t i = 0; i < Event_Pool_Size; i++) {
				mEvents[i].Initialize(scgms::NDevice_Event_Code::Nothing);
				mEvents[i].Set_Slot(i);
				mAllocated_Flags[i] = false;
			}
		}
	
		~CEvent_Pool() {
			for (size_t i = 0; i < Event_Pool_Size; i++) {
				if (mAllocated_Flags[i]) {
					dprintf("Leaked device event; logical time: %d\n", mEvents[i].logical_clock());
				}
			}
		}

		CDevice_Event* Alloc_Event() {
			//obtain working index, but we need to do it as modulo spinlock

			size_t working_idx = mRecent_Allocated_Event_Idx;
			bool locked = false;
			size_t retries_count = Event_Pool_Size * 2;


			while ((!locked) && (retries_count-- > 0)) {
				working_idx = (working_idx + 1) % Event_Pool_Size;
				if (!mAllocated_Flags[working_idx]) {
					locked = !mAllocated_Flags[working_idx].exchange(true);
				}
			}


			if (locked) {
				mRecent_Allocated_Event_Idx.store(working_idx);
				return &mEvents[working_idx];
			}
			else {
				return new CDevice_Event{};	//should be controlled with a flag for embedded devices
			}
		}

		void Free_Event(const size_t slot) {
			if (slot < Event_Pool_Size)
				mAllocated_Flags[slot] = false;
		}
};




CEvent_Pool event_pool;

std::atomic<int64_t> global_logical_time{ 0 };

void Clone_Raw(const scgms::TDevice_Event& src_raw, scgms::TDevice_Event& dst_raw) noexcept {

	memcpy(&dst_raw, &src_raw, sizeof(dst_raw));
	dst_raw.logical_time = global_logical_time.fetch_add(1);

	switch (scgms::UDevice_Event_internal::major_type(dst_raw.event_code)) {
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:
		{
			if (dst_raw.info) {
				dst_raw.info->AddRef();
			}
			break;
		}
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:
		{
			if (dst_raw.parameters) {
				dst_raw.parameters->AddRef();
			}
			break;
		}
		default: break;	//just keeping the checkers happy
	}
}


CDevice_Event::CDevice_Event(CDevice_Event&& other) noexcept {
	memcpy(&mRaw, &other.mRaw, sizeof(mRaw));
	memset(&other.mRaw, 0, sizeof(other.mRaw));
}

void CDevice_Event::Initialize(const scgms::NDevice_Event_Code code) noexcept {
	memset(&mRaw, 0, sizeof(mRaw));
	mRaw.logical_time = global_logical_time.fetch_add(1);
	mRaw.event_code = code;
	mRaw.device_time = Unix_Time_To_Rat_Time(time(nullptr));
	mRaw.segment_id = scgms::Invalid_Segment_Id;

	switch (scgms::UDevice_Event_internal::major_type(code)) {

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:
			mRaw.info = refcnt::WString_To_WChar_Container(nullptr);
			break;

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:
			mRaw.parameters = refcnt::Create_Container<double>(nullptr, nullptr);
			break;

		default:
			mRaw.level = std::numeric_limits<double>::quiet_NaN();
			break;
	}
}


void CDevice_Event::Initialize(const scgms::TDevice_Event *event) noexcept {
	Clean_Up();
	Clone_Raw(*event, mRaw);
}

CDevice_Event::~CDevice_Event() noexcept {
	Clean_Up();
}

void CDevice_Event::Clean_Up() noexcept {
	switch (scgms::UDevice_Event_internal::major_type(mRaw.event_code)) {
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:
		{
			if (mRaw.info) 
				mRaw.info->Release();
			break;
		}
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:
		{
			if (mRaw.parameters)
				mRaw.parameters->Release();
			break;
		}
		default:
			break;
	}

	mRaw.info = nullptr;	//also resets parameters to nullptr
}

ULONG IfaceCalling CDevice_Event::Release() noexcept {
	if (mSlot != std::numeric_limits<size_t>::max()) {
		Clean_Up();
		event_pool.Free_Event(mSlot);
	}
	else {
		delete this;
	}
	return 0;
}

HRESULT IfaceCalling CDevice_Event::Raw(scgms::TDevice_Event **dst) noexcept {
	*dst = &mRaw;
	return S_OK;
}

HRESULT IfaceCalling CDevice_Event::Clone(IDevice_Event** event) const noexcept {

	auto clone = event_pool.Alloc_Event();
	if (clone) {
		Clone_Raw(mRaw, clone->mRaw);
		*event = static_cast<scgms::IDevice_Event*>(clone);
		return S_OK;
	} else
		return E_OUTOFMEMORY;
}

//syntactic sugar 
scgms::IDevice_Event* allocate_device_event(scgms::NDevice_Event_Code code) noexcept {
	auto result = event_pool.Alloc_Event();
	if (result) {
		result->Initialize(code);
	}

	return static_cast<scgms::IDevice_Event*>(result);
}

//SCGMS exported function
DLL_EXPORT HRESULT IfaceCalling create_device_event(scgms::NDevice_Event_Code code, scgms::IDevice_Event * *event) noexcept {
	*event = allocate_device_event(code);
	return *event ? S_OK : E_OUTOFMEMORY;
}