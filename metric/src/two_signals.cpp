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

#include "signal_error.h"
#include "descriptor.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/utils/math_utils.h"
#include "../../../common/utils/string_utils.h"

#include <cmath>
#include <fstream>
#include <numeric>
#include <type_traits>



CTwo_Signals::CTwo_Signals(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CTwo_Signals::~CTwo_Signals() {
}

HRESULT CTwo_Signals::Do_Execute(scgms::UDevice_Event event) {
	scgms::TDevice_Event *raw_event;
	

	auto add_level = [&raw_event, this](TSegment_Signals &signals, const bool is_reference_signal) {
		auto signal = is_reference_signal ? signals.reference_signal : signals.error_signal;

		if (signal) {
			if (signal->Update_Levels(&raw_event->device_time, &raw_event->level, 1) == S_OK) {
				mNew_Data_Logical_Clock++;
			}

			HRESULT rc = On_Level_Added(raw_event->segment_id, raw_event->device_time);
			if (!Succeeded(rc))
				return rc;
		}

		return S_OK;
	};

	auto add_segment = [&raw_event, this, add_level](const bool is_reference_signal) {		
		auto signals = mSignal_Series.find(raw_event->segment_id);
		if (signals == mSignal_Series.end()) {
			TSegment_Signals signals;
			HRESULT rc = add_level(signals, is_reference_signal);
			if (Succeeded(rc))
				mSignal_Series[raw_event->segment_id] = std::move(signals);
			else
				return rc;
		}
		else {
			HRESULT rc = add_level(signals->second, is_reference_signal);
			if (!Succeeded(rc))
				return rc;
		}

		return S_OK;
	};

	if (event->Raw(&raw_event) == S_OK) {
		switch (raw_event->event_code) {
			case scgms::NDevice_Event_Code::Level:
			{
				const bool is_reference_signal = raw_event->signal_id == mReference_Signal_ID;				
				if (is_reference_signal || (raw_event->signal_id == mError_Signal_ID)) {

					if (!(std::isnan(event.level()) || std::isnan(event.device_time()))) {
						std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
						HRESULT rc = add_segment(is_reference_signal);
						if (!Succeeded(rc))
							return rc;
					}
				}

				break;
			}

			case scgms::NDevice_Event_Code::Warm_Reset:
			{
				std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
				mSignal_Series.clear();
				mShutdown_Received = false;
				mNew_Data_Logical_Clock++;
				break;
			}

			/*
				There could be an intriguing situation, which may look like an error.
				If async filtr produces events, it can emit events in a separate thread
				after the shutdown signal - for performance reasons.
				In such a case, it may emit segment stop after shutdown. This will produce
				two metric values, event if it is set to emit single metric value only.
				We can prevent this by moving the shutdown handler to dtor. But, this would break
				the logic that shutdown is the last event processed.
				Hence, we emit nothing after the shutdown.
				
				Anyone truly interested in the cumulative final value, should used the promised metric
				that's called from dtor. This value will include all levels received, regadless
				segment stop and shutdown.
			*/

			case scgms::NDevice_Event_Code::Shut_Down:
				mShutdown_Received = true;				
				Flush_Stats();
				break;

			default:
				break;
		}
	}

	return mOutput.Send(event);
}

HRESULT CTwo_Signals::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mReference_Signal_ID = configuration.Read_GUID(rsReference_Signal, Invalid_GUID);
	mError_Signal_ID = configuration.Read_GUID(rsError_Signal, Invalid_GUID);
	
	if (Is_Invalid_GUID(mReference_Signal_ID, mError_Signal_ID)) return E_INVALIDARG;

	mCSV_Path = configuration.Read_File_Path(rsOutput_CSV_File);

	mDescription = configuration.Read_String(rsDescription, true, GUID_To_WString(mReference_Signal_ID).append(L" - ").append(GUID_To_WString(mError_Signal_ID)));
	
	return S_OK;
}

	
bool CTwo_Signals::Prepare_Levels(const uint64_t segment_id, std::vector<double> &times, std::vector<double> &reference, std::vector<double> &error) {

	auto prepare_levels_per_single_segment = [](TSegment_Signals &signals, std::vector<double>& times, std::vector<double>& reference, std::vector<double>& error)->bool {
		size_t reference_count = 0;

		if (Succeeded(signals.reference_signal->Get_Discrete_Bounds(nullptr, nullptr, &reference_count))) {
			if (reference_count > 0) {

				const size_t offset = times.size();

				times.resize(offset+reference_count);
				reference.resize(offset+reference_count);

				size_t filled;
				if (signals.reference_signal->Get_Discrete_Levels(times.data()+ offset, reference.data()+offset, reference_count, &filled) == S_OK) {
					error.resize(offset+reference_count);

					if (signals.error_signal->Get_Continuous_Levels(nullptr, times.data()+offset, error.data()+offset, filled, scgms::apxNo_Derivation) == S_OK)
						return true;
				}

			}
			else
				return true;	//no error, but simply no data yet
		}
		return false;
	};

	
	bool result = false;	//assume failure
	switch (segment_id) {
		case scgms::Invalid_Segment_Id:  break;	//result is already false
		case scgms::All_Segments_Id: {

			for (auto &signals : mSignal_Series)
				result |= prepare_levels_per_single_segment(signals.second, times, reference, error);	//we want at least one OK segment

			break;
		}

		default: {
			auto signals = mSignal_Series.find(segment_id);
			if (signals != mSignal_Series.end())
				result = prepare_levels_per_single_segment(signals->second, times, reference, error);
		}
	}

	return result;
}

HRESULT IfaceCalling CTwo_Signals::Logical_Clock(ULONG *clock) {
	if (!clock) return E_INVALIDARG;

	const ULONG old_clock = *clock;
	*clock = mNew_Data_Logical_Clock;

	return old_clock < *clock ? S_OK : S_FALSE;
}


HRESULT IfaceCalling CTwo_Signals::Get_Description(wchar_t** const desc) {
	*desc = const_cast<wchar_t*>(mDescription.c_str());
	return S_OK;
}

void CTwo_Signals::Flush_Stats() {
	if (mCSV_Path.empty()) return;

	std::wofstream stats_file{ mCSV_Path };

	if (!stats_file.is_open()) {
		Emit_Info(scgms::NDevice_Event_Code::Error, std::wstring{ dsCannot_Open_File } + mCSV_Path.wstring());
		return;
	}

	Do_Flush_Stats(std::move(stats_file));
}