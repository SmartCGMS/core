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

#include "signal_error.h"
#include "stats.h"
#include "descriptor.h"

#include <cmath>

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"

constexpr unsigned char bool_2_uc(const bool b) {
	return b ? static_cast<unsigned char>(1) : static_cast<unsigned char>(0);
}


CSignal_Error::CSignal_Error(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CSignal_Error::~CSignal_Error() {
	if (mPromised_Metric)
		*mPromised_Metric = Calculate_Metric(mPromised_Segment_id);
}

HRESULT IfaceCalling CSignal_Error::QueryInterface(const GUID*  riid, void ** ppvObj) {

	if (Internal_Query_Interface<scgms::ISignal_Error_Inspection>(scgms::IID_Signal_Error_Inspection, *riid, ppvObj)) return S_OK;	

	return E_NOINTERFACE;
}


HRESULT CSignal_Error::Do_Execute(scgms::UDevice_Event event) {
	scgms::TDevice_Event *raw_event;
	

	auto add_level = [&raw_event, this](TSegment_Signals &signals, const bool is_reference_signal) {
		auto signal = is_reference_signal ? signals.reference_signal : signals.error_signal;

		if (mEmit_Metric_As_Signal) {
			if (raw_event->device_time != mLast_Emmitted_Time) {
				//emit the signal only if the last time has changed to avoid emmiting duplicate values
				if (!std::isnan(mLast_Emmitted_Time)) {
					Emit_Metric_Signal(raw_event->segment_id);
				}
				
				mLast_Emmitted_Time = raw_event->device_time;
			}
		}

		if (signal->Add_Levels(&raw_event->device_time, &raw_event->level, 1) == S_OK) {
			mNew_Data_Available = true;
		}
	};

	auto add_segment = [&raw_event, this, add_level](const bool is_reference_signal) {
		std::lock_guard<std::mutex> lock{ mSeries_Gaurd };

		auto signals = mSignal_Series.find(raw_event->segment_id);
		if (signals == mSignal_Series.end()) {
			TSegment_Signals signals;
			add_level(signals, is_reference_signal);
			mSignal_Series[raw_event->segment_id] = std::move(signals);
		}
		else {
			add_level(signals->second, is_reference_signal);
		}

		
	};

	if (event->Raw(&raw_event) == S_OK) {
		switch (raw_event->event_code) {
			case scgms::NDevice_Event_Code::Level:
			{
				const bool is_reference_signal = raw_event->signal_id == mReference_Signal_ID;				
				if (is_reference_signal || (raw_event->signal_id == mError_Signal_ID))
					add_segment(is_reference_signal);

				break;
			}

			case scgms::NDevice_Event_Code::Warm_Reset:
			{
				std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
				mSignal_Series.clear();
				mNew_Data_Available = true;
				break;
			}



			case scgms::NDevice_Event_Code::Time_Segment_Stop:
				if (mEmit_Last_Value_Only) Emit_Metric_Signal(raw_event->segment_id);
				break;

			default:
				break;
		}
	}

	return Send(event);
}

HRESULT CSignal_Error::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mReference_Signal_ID = configuration.Read_GUID(rsReference_Signal, Invalid_GUID);
	mError_Signal_ID = configuration.Read_GUID(rsError_Signal, Invalid_GUID);
	if ((mReference_Signal_ID == Invalid_GUID) || (mError_Signal_ID == Invalid_GUID)) return E_INVALIDARG;

	mDescription = configuration.Read_String(rsDescription, GUID_To_WString(mReference_Signal_ID).append(L" - ").append(GUID_To_WString(mError_Signal_ID)));

	const scgms::TMetric_Parameters metric_parameters{ configuration.Read_GUID(rsSelected_Metric),
		bool_2_uc(configuration.Read_Bool(rsUse_Relative_Error)),
		bool_2_uc(configuration.Read_Bool(rsUse_Squared_Diff)),
		bool_2_uc(configuration.Read_Bool(rsUse_Prefer_More_Levels)),
		configuration.Read_Double(dsMetric_Threshold, 0.0)
	};
	
	mMetric = scgms::SMetric{ metric_parameters };	

	return S_OK;
}

	
bool CSignal_Error::Prepare_Levels(const uint64_t segment_id, std::vector<double> &times, std::vector<double> &reference, std::vector<double> &error) {	

	auto prepare_levels_per_single_segment = [](TSegment_Signals &signals, std::vector<double>& times, std::vector<double>& reference, std::vector<double>& error)->bool {
		size_t reference_count = 0;

		if (SUCCEEDED(signals.reference_signal->Get_Discrete_Bounds(nullptr, nullptr, &reference_count))) {
			if (reference_count > 0) {

				const size_t offset = times.size();

				times.resize(offset+reference_count);
				reference.resize(offset+reference_count);

				size_t filled;
				if (signals.reference_signal->Get_Discrete_Levels(times.data()+ offset, reference.data()+offset, reference_count, &filled) == S_OK) {
					error.resize(offset+reference_count);

					if (signals.error_signal->Get_Continuous_Levels(nullptr, times.data()+offset, error.data(), filled, scgms::apxNo_Derivation) == S_OK)
						return true;
				}

			}
			else
				return true;	//no error, but simply no data yet
		}
		return false;
	};

	
	bool result = true;	//no error, but simply no data yet
	switch (segment_id) {
		case scgms::Invalid_Segment_Id:  result = false; break;
		case scgms::All_Segments_Id: {

			for (auto &signals : mSignal_Series)
				result &= prepare_levels_per_single_segment(signals.second, times, reference, error);

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

double CSignal_Error::Calculate_Metric(const uint64_t segment_id) {
	double result = std::numeric_limits<double>::quiet_NaN();
	std::vector<double> reference_times;
	std::vector<double> reference_levels;
	std::vector<double> error_levels;

	std::lock_guard<std::mutex> lock{ mSeries_Gaurd };

	//1. calculate the difference against the exact reference levels => get their times
	if (Prepare_Levels(segment_id, reference_times, reference_levels, error_levels)) {
		if (mMetric->Reset() == S_OK) {
			if (mMetric->Accumulate(reference_times.data(), reference_levels.data(), error_levels.data(), reference_times.size()) == S_OK) {
				size_t levels_acquired = 0;

				if (mMetric->Calculate(&result, &levels_acquired, 0) == S_OK)
					if (levels_acquired == 0)
						result = std::numeric_limits<double>::quiet_NaN();
			}
		}
	}
	
	
	return result;
}

HRESULT IfaceCalling CSignal_Error::Promise_Metric(const uint64_t segment_id, double* const metric_value, bool defer_to_dtor) {
	if (!defer_to_dtor) {
		*metric_value = Calculate_Metric(segment_id);
		return std::isnan(*metric_value) ? S_FALSE : S_OK;
	}
	else {
		mPromised_Metric = metric_value;
		mPromised_Segment_id = segment_id;
		return S_OK;
	}
}

HRESULT IfaceCalling CSignal_Error::Peek_New_Data_Available() {
	return mNew_Data_Available.exchange(false) ? S_OK : S_FALSE;
}

HRESULT IfaceCalling CSignal_Error::Calculate_Signal_Error(const uint64_t segment_id, scgms::TSignal_Stats *absolute_error, scgms::TSignal_Stats *relative_error) {
	if (!absolute_error || !relative_error) return E_INVALIDARG;

	std::vector<double> times;
	std::vector<double> reference_levels;
	std::vector<double> error_levels;

	std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
	if (Prepare_Levels(segment_id, times, reference_levels, error_levels)) {

		//let's reuse the already allocated memory
		decltype(error_levels) &absolute_differences = error_levels;			//has to be error level not to overwrite reference too soon
		decltype(reference_levels) &relative_differences = reference_levels;

		//1. calculate sum and count
		size_t absolute_error_count = 0;
		size_t relative_error_count = 0;
		for (size_t i = 0; i < reference_levels.size(); i++) {
			if (!std::isnan(reference_levels[i]) && !std::isnan(error_levels[i])) {
				//both levels are not nan, so we can calcualte the error here
				absolute_differences[absolute_error_count] = std::fabs(reference_levels[i] - error_levels[i]);
				
				if (reference_levels[i] != 0.0) {
					relative_differences[relative_error_count] = absolute_differences[absolute_error_count] / reference_levels[i];
					relative_error_count++;
				}

				absolute_error->count++;
			}
		}
		//2. test the count and if OK, calculate avg and others
		if (!Calculate_Signal_Stats(absolute_differences, *absolute_error)) return S_FALSE;
		Calculate_Signal_Stats(relative_differences, *relative_error);
		
		return S_OK;
	}
	else return E_FAIL;
}

HRESULT IfaceCalling CSignal_Error::Get_Description(wchar_t** const desc) {
	*desc = const_cast<wchar_t*>(mDescription.c_str());
	return S_OK;
}

void CSignal_Error::Emit_Metric_Signal(uint64_t segment_id) {
	scgms::UDevice_Event event{scgms::NDevice_Event_Code::Level};

	event.device_id() = event.signal_id() = signal_error::metric_signal_id;
	event.level() = Calculate_Metric(segment_id);
	event.segment_id() = segment_id;

	Send(event);
}