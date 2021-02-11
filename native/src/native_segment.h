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

#pragma once

#include "descriptor.h"

#include "../../../common/iface/NativeIface.h"
#include "../../../common/rtl/FilterLib.h"

#include <array>
#include <map>

class CNative_Segment {
protected:
	TNative_Environment mEnvironment;
	std::array<double, native::max_signal_count> mPrevious_Device_Time, mLast_Device_Time;
	std::array<double, native::max_signal_count> mPrevious_Level, mLast_Level;
	//Although the mLast_* arrays duplicate the respective environment arrays, we keep the duplicities
	//to recover from possibly faulty script, which could rewrite the environment

	std::vector<unsigned char> mState_Container;
protected:
	double mRecent_Time = std::numeric_limits<double>::quiet_NaN();	//time for Send_Event
	uint64_t mSegment_Id;
	bool mSync_To_Any = false;
	scgms::SFilter mOutput;	//aka the next_filter
	TNative_Execute_Wrapper mEntry_Point;

	HRESULT Send_Event(const GUID* sig_id, const double device_time, const double level, const char* msg);
	void Emit_Info(const bool is_error, const std::wstring& msg);
protected:
	friend HRESULT IfaceCalling Send_Handler(const GUID* sig_id, const double device_time, const double level, const char* msg, const void* context);
public:
	CNative_Segment(scgms::SFilter output, const uint64_t segment_id, TNative_Execute_Wrapper entry_point,
		const std::array<GUID, native::max_signal_count>& signal_ids, const std::array<double, native::max_parameter_count> &parameters,
		const size_t custom_data_size);
	HRESULT Execute(const size_t signal_idx, GUID& signal_id, double& device_time, double& level) noexcept;
};

HRESULT IfaceCalling Send_Handler(const GUID* sig_id, const double device_time, const double level, const char* msg, const void* context);