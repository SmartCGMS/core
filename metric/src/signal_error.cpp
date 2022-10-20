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
#include <numeric>
#include <type_traits>


constexpr unsigned char bool_2_uc(const bool b) {
	return b ? static_cast<unsigned char>(1) : static_cast<unsigned char>(0);
}


CSignal_Error::CSignal_Error(scgms::IFilter *output) : CBase_Filter(output), CTwo_Signals(output) {
	//
}

CSignal_Error::~CSignal_Error() {
	if (mPromised_Metric) {
		std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
		*mPromised_Metric = Calculate_Metric(mPromised_Segment_id);
	}
}

HRESULT IfaceCalling CSignal_Error::QueryInterface(const GUID*  riid, void ** ppvObj) {

	if (Internal_Query_Interface<scgms::ISignal_Error_Inspection>(scgms::IID_Signal_Error_Inspection, *riid, ppvObj)) return S_OK;	

	return E_NOINTERFACE;
}


HRESULT CSignal_Error::On_Level_Added(const uint64_t segment_id, const double device_time) {
	HRESULT rc = S_OK;

	if (mEmit_Metric_As_Signal && !mEmit_Last_Value_Only) {
		if (device_time != mLast_Emmitted_Time) {
			//emit the signal only if the last time has changed to avoid emmiting duplicate values
			if (!std::isnan(mLast_Emmitted_Time)) {
				rc = Emit_Metric_Signal(segment_id, mLast_Emmitted_Time);
			}

			mLast_Emmitted_Time = device_time;
		}
	}

	return rc;
}

HRESULT CSignal_Error::Do_Execute(scgms::UDevice_Event event) {
	const auto device_time = event.device_time();
	const auto segment_id = event.segment_id();
	const auto event_code = event.event_code();

	HRESULT rc = S_OK;
	
	switch (event_code) {
			
		case scgms::NDevice_Event_Code::Time_Segment_Stop:
			if (mEmit_Metric_As_Signal && mEmit_Last_Value_Only && !mShutdown_Received) {
				std::lock_guard<std::mutex> lock{ mSeries_Gaurd };

				auto signals = mSignal_Series.find(segment_id);
				if (signals != mSignal_Series.end()) {
					rc = Emit_Metric_Signal(segment_id, device_time);
					signals->second.last_value_emitted = Succeeded(rc);
				}
			}
			break;

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
			if (mEmit_Metric_As_Signal && mEmit_Last_Value_Only) {
				std::lock_guard<std::mutex> lock{ mSeries_Gaurd };

				//emit any last value, which we have not emitted due to missing segment stop marker
				for (auto& signals: mSignal_Series) {

					if (!signals.second.last_value_emitted) {
						rc = Emit_Metric_Signal(signals.first, device_time);
						if (Succeeded(rc))
							signals.second.last_value_emitted = true;
						else
							break;
					}
				}

				if (Succeeded(rc))
					rc = Emit_Metric_Signal(scgms::All_Segments_Id, device_time);
			}			
				
			break;

		default:
			rc = S_OK;
			break;
	}		
	
	//eventually, execute the terminal events
	scgms::IDevice_Event* raw = event.get();
	event.release();
	if (Succeeded(rc))
		return CTwo_Signals::Do_Execute(scgms::UDevice_Event{ raw });	//why just cannot we pass std::move(event)?
	else
		return rc;
}

HRESULT CSignal_Error::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	const HRESULT rc = CTwo_Signals::Do_Configure(configuration, error_description);
	if (!Succeeded(rc)) return rc;

	const GUID metric_id = configuration.Read_GUID(rsSelected_Metric);
	const double metric_threshold = configuration.Read_Double(rsMetric_Threshold);
	
	if (Is_Invalid_GUID(metric_id)) {
		std::wstring err_desc = dsInvalid_Metric_GUID;
		err_desc += configuration.Read_String(rsSelected_Metric);
		error_description.push(err_desc);
		return E_INVALIDARG;
	}
		
	if (std::isnan(metric_threshold)) 
		return E_INVALIDARG;
	

	mEmit_Metric_As_Signal = configuration.Read_Bool(rsEmit_metric_as_signal, mEmit_Metric_As_Signal);
	mEmit_Last_Value_Only = configuration.Read_Bool(rsEmit_last_value_only, mEmit_Last_Value_Only);

	mLevels_Required = configuration.Read_Int(rsMetric_Levels_Required, mLevels_Required);

	mDescription = configuration.Read_String(rsDescription, true, GUID_To_WString(mReference_Signal_ID).append(L" - ").append(GUID_To_WString(mError_Signal_ID)));

	
	const scgms::TMetric_Parameters metric_parameters{ metric_id,
		bool_2_uc(configuration.Read_Bool(rsUse_Relative_Error)),
		bool_2_uc(configuration.Read_Bool(rsUse_Squared_Diff)),
		bool_2_uc(configuration.Read_Bool(rsUse_Prefer_More_Levels)),
		metric_threshold
	};
	
	mMetric = scgms::SMetric{ metric_parameters };	

	return S_OK;
}

	

double CSignal_Error::Calculate_Metric(const uint64_t segment_id) {
	double result = std::numeric_limits<double>::quiet_NaN();
	std::vector<double> reference_times;
	std::vector<double> reference_levels;
	std::vector<double> error_levels;

	
	//1. calculate the difference against the exact reference levels => get their times
	if (Prepare_Levels(segment_id, reference_times, reference_levels, error_levels)) {
		if (mMetric->Reset() == S_OK) {
			if (mMetric->Accumulate(reference_times.data(), reference_levels.data(), error_levels.data(), reference_times.size()) == S_OK) {
				size_t levels_acquired = 0;

				if (mMetric->Calculate(&result, &levels_acquired, mLevels_Required) == S_OK)
					if (levels_acquired == 0)
						result = std::numeric_limits<double>::quiet_NaN();
			}
		}
	}
	
	
	return result;
}

HRESULT IfaceCalling CSignal_Error::Promise_Metric(const uint64_t segment_id, double* const metric_value, BOOL defer_to_dtor) {
	std::lock_guard<std::mutex> lock{ mSeries_Gaurd };

	if (defer_to_dtor == FALSE) {
		*metric_value = Calculate_Metric(segment_id);
		return std::isnan(*metric_value) ? S_FALSE : S_OK;
	}
	else {
		mPromised_Metric = metric_value;
		mPromised_Segment_id = segment_id;
		return S_OK;
	}
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
				const double abs_diff = std::fabs(reference_levels[i] - error_levels[i]);
				absolute_differences[absolute_error_count] = abs_diff;
				
				if (reference_levels[i] != 0.0) {
					relative_differences[relative_error_count] = abs_diff / reference_levels[i];
					relative_error_count++;
				}
				else if (abs_diff == 0.0) {	//special-case, when the relative error is zero even if the reference level could be zero
					relative_differences[relative_error_count] = 0.0;
					relative_error_count++;
				}

				absolute_error_count++;
			}
		}
		absolute_differences.resize(absolute_error_count);
		relative_differences.resize(relative_error_count);

		//2. test the count and if OK, calculate avg and others
		if (!Calculate_Signal_Stats(absolute_differences, *absolute_error)) return S_FALSE;
		Calculate_Signal_Stats(relative_differences, *relative_error);
		
		return S_OK;
	}
	else return E_FAIL;
}

HRESULT CSignal_Error::Emit_Metric_Signal(const uint64_t segment_id, const double device_time) {
	scgms::UDevice_Event event{scgms::NDevice_Event_Code::Level};
	if (event) {

		event.device_id() = event.signal_id() = signal_error::metric_signal_id;
		event.level() = Calculate_Metric(segment_id);
		event.segment_id() = segment_id;
		event.device_time() = device_time;

		return mOutput.Send(event);
	}
	else
		return E_OUTOFMEMORY;
}

void CSignal_Error::Do_Flush_Stats(std::wofstream stats_file) {
	using et = std::underlying_type < scgms::NECDF>::type;

	stats_file << rsSignal_Stats_Header;

	//append rest of the header for ECDF
	const wchar_t *tc_d = L";; ";
	stats_file << tc_d << static_cast<et>(scgms::NECDF::min_value) ;
	for (et i = static_cast<et>(scgms::NECDF::min_value) + 1; i <= static_cast<et>(scgms::NECDF::max_value); i++)
		stats_file << "; " << i;
	stats_file << std::endl;


	auto flush_stats = [this, &stats_file, &tc_d](const scgms::TSignal_Stats& signal_stats, const wchar_t *marker_string, const uint64_t segment_id) {
		

		if (segment_id == scgms::All_Segments_Id)  stats_file << dsSelect_All_Segments;
			else stats_file << std::to_wstring(segment_id);

		stats_file << "; " << marker_string << tc_d
			<< signal_stats.avg << "; " << signal_stats.stddev << "; " << signal_stats.count << tc_d
			<< signal_stats.ecdf[scgms::NECDF::min_value] << "; "
			<< signal_stats.ecdf[scgms::NECDF::p25] << "; "
			<< signal_stats.ecdf[scgms::NECDF::median] << "; "
			<< signal_stats.ecdf[scgms::NECDF::p75] << "; "
			<< signal_stats.ecdf[scgms::NECDF::p95] << "; "
			<< signal_stats.ecdf[scgms::NECDF::p99] << "; "
			<< signal_stats.ecdf[scgms::NECDF::max_value];
		
		//write ECDF
		stats_file << tc_d << signal_stats.ecdf[scgms::NECDF::min_value];
		for (et i = static_cast<et>(scgms::NECDF::min_value) + 1; i <= static_cast<et>(scgms::NECDF::max_value); i++)
			stats_file << "; " << signal_stats.ecdf[static_cast<scgms::NECDF>(i)];

		stats_file<< std::endl;
	};

	auto flush_segment = [this, &stats_file, &flush_stats](const uint64_t segment_id) {
		scgms::TSignal_Stats absolute_error;
		scgms::TSignal_Stats relative_error;
		if (Calculate_Signal_Error(segment_id, &absolute_error, &relative_error) == S_OK) {
			flush_stats(absolute_error, dsAbsolute, segment_id);
			flush_stats(relative_error, dsRelative, segment_id);
			
		}
	};

	for (auto& signals: mSignal_Series) 
		flush_segment(signals.first);
	
	flush_segment(scgms::All_Segments_Id);
}