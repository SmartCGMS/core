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

#include "native_segment.h"

#include <utils/math_utils.h>
#include <utils/string_utils.h>

CNative_Segment::CNative_Segment(scgms::SFilter output, const uint64_t segment_id, TNative_Execute_Wrapper entry_point,
									const std::array<GUID, native::required_signal_count>& signal_ids) :
	mSegment_Id(segment_id), mOutput(output), mEntry_Point(entry_point) {


	mEnvironment.send = nullptr;
	mEnvironment.custom_data = nullptr;

	mEnvironment.level_count = native::required_signal_count;
	for (size_t i = 0; i < native::required_signal_count; i++) {
		mEnvironment.signal_id[i] = signal_ids[i];
		mEnvironment.device_time[i] = mEnvironment.level[i] = mEnvironment.slope[i] = std::numeric_limits<double>::quiet_NaN();
		mPrevious_Device_Time[i] = mLast_Device_Time[i] = mPrevious_Level[i] = mLast_Level[i] = std::numeric_limits<double>::quiet_NaN();
	}

	mEnvironment.parameter_count = 0;
	mEnvironment.parameters = nullptr;
}

void CNative_Segment::Emit_Info(const bool is_error, const std::wstring& msg) {
	scgms::UDevice_Event event{ is_error ? scgms::NDevice_Event_Code::Error : scgms::NDevice_Event_Code::Information };
	event.device_id() = native::native_filter_id;
	event.info.set(msg.c_str());
	event.segment_id() = mSegment_Id;
	event.device_time() = mRecent_Time;
	mOutput.Send(event);
}

HRESULT CNative_Segment::Execute(const size_t signal_idx, GUID& signal_id, double& device_time, double& level) {
	mRecent_Time = device_time;

	mPrevious_Device_Time[signal_idx] = mLast_Device_Time[signal_idx];
	mPrevious_Level[signal_idx] = mLast_Level[signal_idx];
	
	mEnvironment.signal_id[signal_idx] = signal_id;	
	mEnvironment.device_time[signal_idx] = mLast_Device_Time[signal_idx] = device_time;
	mEnvironment.level[signal_idx] = mLast_Level[signal_idx] = level;

	mEnvironment.current_signal_index = signal_idx;

	if (!Is_Any_NaN(mPrevious_Device_Time[signal_idx], mLast_Device_Time[signal_idx], mPrevious_Level[signal_idx], mLast_Level[signal_idx])) {
		const double dx = mLast_Device_Time[signal_idx] - mPrevious_Device_Time[signal_idx];
		if (dx>0.0)
			mEnvironment.slope[signal_idx] = (mLast_Level[signal_idx] - mPrevious_Level[signal_idx]) / dx;
		else
			//time must advance forward only
			mEnvironment.slope[signal_idx] = std::numeric_limits<double>::quiet_NaN();
	} else {
		mEnvironment.slope[signal_idx] = std::numeric_limits<double>::quiet_NaN();
	}

	HRESULT rc = S_OK;
	try {
		rc = mEntry_Point(&signal_id, &device_time, &level, &mEnvironment, this);
	}
	catch (const std::exception& ex) {
		// specific handling for all exceptions extending std::exception, except
		// std::runtime_error which is handled explicitly
		std::wstring error_desc = Widen_Char(ex.what());
		Emit_Info(true, error_desc);
		rc = E_FAIL;
	}
	catch (...) {
		Emit_Info(true, L"Unknown error!");
		rc = E_FAIL;
	}

	return rc;
}

HRESULT CNative_Segment::Send_Event(const GUID* sig_id, const double device_time, const double level, const char* msg) {
	HRESULT rc = E_UNEXPECTED;

	if (msg == nullptr) {
		//we are emitting a level event
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
		evt.device_id() = native::native_filter_id;
		evt.device_time() = mRecent_Time;
		evt.level() = level;
		evt.segment_id() = mSegment_Id;

		rc = mOutput.Send(evt);
	}
	else {
		//we are emitting an info event
		std::wstring wmsg{Widen_String(msg)};		
		Emit_Info(std::isnan(level), wmsg);
	}
	
	return rc;

}