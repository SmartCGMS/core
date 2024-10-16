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

#include "signal_stats.h"

#include <scgms/lang/dstrings.h>
#include <scgms/rtl/UILib.h>
#include <scgms/utils/string_utils.h>
#include <scgms/utils/math_utils.h>


#include <fstream>
#include <numeric>
#include <type_traits>

CSignal_Stats::CSignal_Stats(scgms::IFilter* output) : CBase_Filter(output) {
}

CSignal_Stats::~CSignal_Stats() {
}

HRESULT CSignal_Stats::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mSignal_ID = configuration.Read_GUID(rsSelected_Signal);
	if (mSignal_ID == Invalid_GUID) {
		return E_INVALIDARG;
	}

	mCSV_Path = configuration.Read_File_Path(rsOutput_CSV_File);
	mDiscard_Repeating_Level = configuration.Read_Bool(rsDiscard_Repeating_Level, mDiscard_Repeating_Level);
		
	if (mCSV_Path.empty()) {
		error_description.push(dsOutput_to_file_enabled_but_no_filename_given);
		return E_INVALIDARG;    //remove this as an error, once we implement inspection iface
	}

	return S_OK; 
}

HRESULT CSignal_Stats::Do_Execute(scgms::UDevice_Event event) {
	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Warm_Reset:
			mSignal_Series.clear();
			break;

		case scgms::NDevice_Event_Code::Shut_Down:
			Flush_Stats();
			break;

		default: break;
	}

	if (event.signal_id() == mSignal_ID) {
		if (event.event_code() == scgms::NDevice_Event_Code::Level) { //we intentionally ignore masked levels
			const double level = event.level();
			const double date_time = event.device_time();

			if (!(std::isnan(level) || std::isnan(date_time))) {

				auto series = mSignal_Series.find(event.segment_id());

				if (series == mSignal_Series.end()) {
					TLevels levels;
					levels.last_level = level;
					levels.level.push_back(level);
					levels.datetime.push_back(date_time);
					mSignal_Series[event.segment_id()] = std::move(levels);
				}
				else {
					auto& levels = series->second;

					const bool discard_level = mDiscard_Repeating_Level && (levels.last_level == level);
					if (!discard_level) {
						levels.last_level = level;
						levels.level.push_back(level);
						levels.datetime.push_back(date_time);
					}
				}
			}

		}
	}

	return mOutput.Send(event);
}

void CSignal_Stats::Flush_Stats() {

	std::wofstream stats_file{ mCSV_Path };
	if (!stats_file.is_open()) {
		Emit_Info(scgms::NDevice_Event_Code::Error, std::wstring{ dsCannot_Open_File } + mCSV_Path.wstring());
		return;
	}

	//write the header
	{
		scgms::CSignal_Description signal_names;
		stats_file << signal_names.Get_Name(mSignal_ID) << "; " << GUID_To_WString(mSignal_ID) << std::endl;
		stats_file << rsSignal_Stats_Header << std::endl;
	}

	auto flush_marker = [&stats_file](const uint64_t segment_id, auto& series, const wchar_t* marker_string) {
		scgms::TSignal_Stats signal_stats;

		const bool result = Calculate_Signal_Stats(series, signal_stats);

		if (result) {
			if (segment_id == scgms::All_Segments_Id) {
				stats_file << dsSelect_All_Segments;
			}
			else {
				stats_file << std::to_wstring(segment_id);
			}

			stats_file << "; " << marker_string << ";; "
				<< signal_stats.avg << "; " << signal_stats.stddev << "; " << signal_stats.exc_kurtosis << "; " << signal_stats.skewness << "; " << signal_stats.count << ";; "
				<< signal_stats.ecdf[scgms::NECDF::min_value] << "; "
				<< signal_stats.ecdf[scgms::NECDF::p25] << "; "
				<< signal_stats.ecdf[scgms::NECDF::median] << "; "
				<< signal_stats.ecdf[scgms::NECDF::p75] << "; "
				<< signal_stats.ecdf[scgms::NECDF::p95] << "; "
				<< signal_stats.ecdf[scgms::NECDF::p99] << "; "
				<< signal_stats.ecdf[scgms::NECDF::max_value] << std::endl;
		}

		return result;
	};

	std::vector<double> total_levels;   //all levels accross all segments
	std::vector<double> total_periods;   //all periods accross all segments

	//accumulate data for total stats, while flushing partial stats per segments
	for (const auto& segment: mSignal_Series) { //signal must be const because we can FlushStats in later implementations sooner than in dtor             
		auto& levels = segment.second;

		if (levels.level.size() > 0) {
			//1. calculate statistics per level
			std::vector<double> levels_stats{ levels.level };   //we need a copy due to the sort to find ECDF
			if (flush_marker(segment.first, levels_stats, dsLevel)) {
				std::move(levels.level.begin(), levels.level.end(), std::back_inserter(total_levels));

				//2. calculate the periods
				if (levels.datetime.size() > 1) {
					std::vector<double> periods{ levels.datetime };
					std::sort(periods.begin(), periods.end());  //we do not need to synchronize time with the levels
					//transform datetimes to periods
					std::adjacent_difference(periods.begin(), periods.end(), periods.begin());
					periods.erase(periods.begin());

					if (flush_marker(segment.first, periods, dsPeriod)) {
						std::move(periods.begin(), periods.end(), std::back_inserter(total_periods));
					}
				}
			}

		}
	}

	stats_file << std::endl;

	if (flush_marker(scgms::All_Segments_Id, total_levels, dsLevel)) {
		flush_marker(scgms::All_Segments_Id, total_periods, dsPeriod);
	}

	stats_file << std::endl;
	stats_file << std::endl;

	stats_file << "Expected excessive kurtosis and skewness per distribution\n";
	stats_file << "Uniform exc.kurt.:;" << -6.0 / 5.0 << "; Skewness; 0\n";
	stats_file << "Normal exc.kurt:;0;Skewness;0\n";
	stats_file << "Exponential exc.kurt:;6;Skewness;2\n";
	stats_file << "Poisson exc.kurt:;1/std.dev;Skewness;1/std.dev^0.5\n";
}
