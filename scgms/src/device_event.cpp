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

#include "device_event.h"

#include "../../../common/rtl/rattime.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/utils/DebugHelper.h"

#include <atomic>
#include <stdexcept>


#include <array>
#include <atomic>
#include <tuple>


constexpr size_t Event_Pool_Size = 1024;

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

		return new CDevice_Event();

		//obtain working index, but we need to do it as modulo spinlock

		size_t working_idx = mRecent_Allocated_Event_Idx;
		bool locked = false;
		size_t retries_count = Event_Pool_Size * 2;


		while ((!locked) && (retries_count-- > 0)) {
			working_idx = (working_idx + 1) % Event_Pool_Size;
			if (!mAllocated_Flags[working_idx])
				locked = !mAllocated_Flags[working_idx].exchange(true);
		}


		if (locked) {
			mRecent_Allocated_Event_Idx.store(working_idx);
			return &mEvents[working_idx];
		}
		else 			
			return nullptr;		
		
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
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:			dst_raw.info->AddRef();
			break;

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:	dst_raw.parameters->AddRef();
			break;

		default: break;	//just keeping the checkers happy
	}
}



void CDevice_Event::Initialize(const scgms::NDevice_Event_Code code) noexcept {
	memset(&mRaw, 0, sizeof(mRaw));
	mRaw.logical_time = global_logical_time.fetch_add(1);
	mRaw.event_code = code;
	mRaw.device_time = Unix_Time_To_Rat_Time(time(nullptr));
	mRaw.segment_id = scgms::Invalid_Segment_Id;

	switch (scgms::UDevice_Event_internal::major_type(code)) {		
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info: mRaw.info = refcnt::WString_To_WChar_Container(nullptr);
			break;

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters: mRaw.parameters = refcnt::Create_Container<double>(nullptr, nullptr);
			break;

		default:mRaw.level = std::numeric_limits<double>::quiet_NaN();
			break;
	}
}


void CDevice_Event::Initialize(const scgms::TDevice_Event *event) noexcept {
	Clone_Raw(*event, mRaw);
}

CDevice_Event::~CDevice_Event() noexcept {
	Clean_Up();
}

void CDevice_Event::Clean_Up() noexcept {
	switch (scgms::UDevice_Event_internal::major_type(mRaw.event_code)) {
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:			if (mRaw.info) 
																						mRaw.info->Release();
																					break;

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:	if (mRaw.parameters) 
																						mRaw.parameters->Release();
																					break;
		default:	break;
	}

	mRaw.info = nullptr;	//also resets parameters to nullptr

}

ULONG IfaceCalling CDevice_Event::Release() noexcept {
	Clean_Up();
	event_pool.Free_Event(mSlot);
	return 0;
}

HRESULT IfaceCalling CDevice_Event::Raw(scgms::TDevice_Event **dst) noexcept {
	*dst = &mRaw;
	return S_OK;
}


HRESULT IfaceCalling CDevice_Event::Clone(IDevice_Event** event) const noexcept {

	auto clone = event_pool.Alloc_Event();
	if (clone)
		Clone_Raw(mRaw, clone->mRaw);

	*event = static_cast<scgms::IDevice_Event*>(clone);
	

	return *event ? S_OK : E_OUTOFMEMORY;
}

//syntactic sugar 
scgms::IDevice_Event* allocate_device_event(scgms::NDevice_Event_Code code) noexcept {
	auto result = event_pool.Alloc_Event();
	if (result)
		result->Initialize(code);

	return static_cast<scgms::IDevice_Event*>(result);
}

//SCGMS exported function
extern "C" HRESULT IfaceCalling create_device_event(scgms::NDevice_Event_Code code, scgms::IDevice_Event **event) noexcept {
	*event = allocate_device_event(code);
	return *event ? S_OK : E_OUTOFMEMORY;
}