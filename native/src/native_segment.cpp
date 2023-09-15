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

#include "native_segment.h"

#include <scgms/utils/math_utils.h>
#include <scgms/utils/string_utils.h>

HRESULT IfaceCalling Send_Handler(const GUID* sig_id, const double device_time, const double level, const char* msg, const void* context) {
	CNative_Segment* segment = reinterpret_cast<CNative_Segment*>(const_cast<void*>(context));

	return segment->Send_Event(sig_id, device_time, level, msg);
}

CNative_Segment::CNative_Segment(scgms::SFilter output, const uint64_t segment_id, TNative_Execute_Wrapper entry_point,
	const std::array<GUID, native::max_signal_count>& signal_ids, const std::array<double, native::max_parameter_count>& parameters,
	const size_t custom_data_size, const bool sync_to_any_signal) :
	mSegment_Id(segment_id), mOutput(output), mEntry_Point(entry_point) {


	mEnvironment.send = &Send_Handler;

	mState_Container.resize(custom_data_size);
	mEnvironment.custom_data = mState_Container.data();

	mEnvironment.level_count = native::max_signal_count;
	for (size_t i = 0; i < native::max_signal_count; i++) {
		mEnvironment.signal_id[i] = signal_ids[i];
		mEnvironment.device_time[i] = mEnvironment.level[i] = mEnvironment.slope[i] = std::numeric_limits<double>::quiet_NaN();
		mPrevious_Device_Time[i] = mLast_Device_Time[i] = mPrevious_Level[i] = mLast_Level[i] = std::numeric_limits<double>::quiet_NaN();
	}

	std::copy(parameters.begin(), parameters.end(), mEnvironment.parameters);

	mSync_To_Any = sync_to_any_signal;
}

HRESULT CNative_Segment::Emit_Info(const bool is_error, const std::wstring& msg) {
	scgms::UDevice_Event event{ is_error ? scgms::NDevice_Event_Code::Error : scgms::NDevice_Event_Code::Information };
	if (event) {
		event.device_id() = native::native_filter_id;
		event.info.set(msg.c_str());
		event.segment_id() = mSegment_Id;
		event.device_time() = mRecent_Time;
		return mOutput.Send(event);
	}
	else
		return E_OUTOFMEMORY;
}

HRESULT CNative_Segment::Execute(const size_t signal_idx, GUID& signal_id, double& device_time, double& level) noexcept {
	mRecent_Time = device_time;

	mEnvironment.current_signal_index = signal_idx;

	const bool user_set_signal = signal_idx < native::max_signal_count;

	if (user_set_signal) {
		mPrevious_Device_Time[signal_idx] = mLast_Device_Time[signal_idx];
		mPrevious_Level[signal_idx] = mLast_Level[signal_idx];

		mEnvironment.signal_id[signal_idx] = signal_id;
		mEnvironment.device_time[signal_idx] = mLast_Device_Time[signal_idx] = device_time;
		mEnvironment.level[signal_idx] = mLast_Level[signal_idx] = level;

		if (!Is_Any_NaN(mPrevious_Device_Time[signal_idx], mLast_Device_Time[signal_idx], mPrevious_Level[signal_idx], mLast_Level[signal_idx])) {
			const double dx = mLast_Device_Time[signal_idx] - mPrevious_Device_Time[signal_idx];
			if (dx > 0.0)
				mEnvironment.slope[signal_idx] = (mLast_Level[signal_idx] - mPrevious_Level[signal_idx]) / dx;
			else
				//time must advance forward only
				mEnvironment.slope[signal_idx] = std::numeric_limits<double>::quiet_NaN();
		}
		else {
			mEnvironment.slope[signal_idx] = std::numeric_limits<double>::quiet_NaN();
		}
	}

	HRESULT rc = S_OK;

	if (mSync_To_Any || user_set_signal) {	
			rc = mEntry_Point(&signal_id, &device_time, &level, &mEnvironment, this);
	}

	return rc;
}

HRESULT CNative_Segment::Send_Event(const GUID* sig_id, const double device_time, const double level, const char* msg) {
	HRESULT rc = E_UNEXPECTED;

	if (msg == nullptr) {
		//we are emitting a level event
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

		if (evt) {

			evt.device_id() = native::native_filter_id;
			evt.device_time() = device_time;
			evt.level() = level;
			evt.segment_id() = mSegment_Id;
			evt.signal_id() = *sig_id;

			rc = mOutput.Send(evt);
		}
		else
			rc = E_OUTOFMEMORY;
	}
	else {
		//we are emitting an info event
		std::wstring wmsg{ Widen_String(msg) };
		rc = Emit_Info(std::isnan(level), wmsg);
	}

	return rc;

}