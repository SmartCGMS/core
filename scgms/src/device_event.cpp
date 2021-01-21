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

#include <atomic>
#include <stdexcept>

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



TEmbedded_Error CDevice_Event::Initialize(scgms::NDevice_Event_Code code) noexcept {
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

	return { S_OK, nullptr };
}


TEmbedded_Error CDevice_Event::Initialize(scgms::IDevice_Event *event) noexcept {
	scgms::TDevice_Event *src_raw;

	const HRESULT rc = event->Raw(&src_raw);
	wchar_t* desc = nullptr;

	if (rc == S_OK)  Clone_Raw(*src_raw, mRaw);
		else desc = L"Cannot get source event!";

	return { rc, desc };
}

CDevice_Event::~CDevice_Event() {
	switch (scgms::UDevice_Event_internal::major_type(mRaw.event_code)) {
		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::info:			if (mRaw.info) mRaw.info->Release();
																						break;

		case scgms::UDevice_Event_internal::NDevice_Event_Major_Type::parameters:		if (mRaw.parameters) mRaw.parameters->Release();
																						break;
		default:	break;
	}

}

ULONG IfaceCalling CDevice_Event::Release() noexcept {
	delete this;
	return 0;
}

HRESULT IfaceCalling CDevice_Event::Raw(scgms::TDevice_Event **dst) noexcept {
	*dst = &mRaw;
	return S_OK;
}


HRESULT IfaceCalling CDevice_Event::Clone(IDevice_Event** event) noexcept {
	HRESULT rc = E_UNEXPECTED;

	std::unique_ptr<CDevice_Event> clone = std::make_unique<CDevice_Event>();
	if (clone) {
		const auto err = clone->Initialize(mRaw.event_code);
		rc = err.code;
		if (Succeeded(rc)) {
			Clone_Raw(mRaw, clone->mRaw);
			*event = clone.get();
			clone.release();

			rc = S_OK;
		}		
	}
	else
		rc = E_OUTOFMEMORY;

	return rc;
}

HRESULT IfaceCalling create_device_event(scgms::NDevice_Event_Code code, scgms::IDevice_Event **event) noexcept {
	std::unique_ptr<CDevice_Event> new_event = std::make_unique<CDevice_Event>();
	if (new_event) {
		const auto err = new_event->Initialize(code);
		*event = Succeeded(err.code) ? static_cast<scgms::IDevice_Event*> (new_event.get()) : nullptr;
		new_event.release();
		return err.code;
	}
	else
		return E_OUTOFMEMORY;
}


