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

#include "log_replay.h"
#include "log.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/utils/string_utils.h"

#include "log.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <ctime>
#include <vector>
#include <algorithm> 
#include <cctype>
#include <cmath>
#include <tuple>

CLog_Replay_Filter::CLog_Replay_Filter(scgms::IFilter* output) : CBase_Filter(output) {
	//
}

CLog_Replay_Filter::~CLog_Replay_Filter() {
	if (mLog_Replay_Thread)
		if (mLog_Replay_Thread->joinable())
			mLog_Replay_Thread->join();
}


void CLog_Replay_Filter::Replay_Log(const std::filesystem::path& log_filename) {
	if (log_filename.empty()) return;

	std::wifstream log{ log_filename.string().c_str() };
	if (!log.is_open()) return;
	

	uint64_t filename_segment_id = scgms::Invalid_Segment_Id;
	if (mInterpret_Filename_As_Segment_Id) {
		std::string name = log_filename.stem().string();
		char* end_char = nullptr;
		uint64_t tmp_id = std::strtoull(name.c_str(), &end_char, 10);
		if (*end_char == 0) filename_segment_id = tmp_id;	//strtoull encoutered digits only, so conversio is OK and we can interpret it as a valid segment id
	}
	const bool filename_segment_id_available = mInterpret_Filename_As_Segment_Id && (filename_segment_id != scgms::Invalid_Segment_Id);


	auto locale = log.imbue(std::locale(std::cout.getloc(), new logger::DecimalSeparator<char>('.')));
	

	std::wstring line;
	std::wstring column;
	size_t line_counter = 0;

	// read header and validate
	// NOTE: this assumes identical header (as the one used when generating log); maybe we could consider adaptive log parsing later
	if (!std::getline(log, line) || trim(line) != std::wstring(dsLog_Header)) {
		std::wstring msg{ dsFile_Has_Not_Expected_Header };
		msg += log_filename.wstring();
		Emit_Info(scgms::NDevice_Event_Code::Warning, msg, filename_segment_id);	
		log.seekg(0);	//so that the following getline extracts the very same string
	}
	else
		line_counter = 1;

	// cuts a single column from input line
	auto cut_column = [&line]() -> std::wstring {
		std::wstring retstr{ L"" };
		
		auto pos = line.find(rsLog_CSV_Separator);
		if (pos != std::string::npos) {
			retstr = line.substr(0, pos);
			line.erase(0, pos + wcslen(rsLog_CSV_Separator));
		}
		else retstr = line;
		
		return trim(retstr);
	};
	

	auto emit_parsing_exception_w = [this, &log_filename, line_counter](const std::wstring &what) {	

		std::wstring msg{ dsUnexpected_Error_While_Parsing };
		msg += log_filename.wstring();		
		msg.append(dsLine_No);
		msg.append(std::to_wstring(line_counter));
		
		Emit_Info(scgms::NDevice_Event_Code::Error, msg);
		if (!what.empty()) Emit_Info(scgms::NDevice_Event_Code::Error, what);
	};

	auto emit_parsing_exception = [&emit_parsing_exception_w](const char* what) {
		emit_parsing_exception_w(Widen_Char(what));
	};

	std::vector<TLog_Entry> log_lines;	//first, we fetch the lines, and then we parse them
				//we do so, to ensure that all the events are time sorted and and events't logical clock are monotonically increasing

	// read all lines from log file
	while (std::getline(log, line))  {
		line_counter++;

		trim(line);
		if (line.empty()) continue;

		try
		{
			// skip; logical time is not modifiable, and there's not a point in loading it anyways
			auto specificval = cut_column();
			// device time is parsed as-is using the same format as used when saving
			specificval = cut_column();
			const double device_time = Local_Time_WStr_To_Rat_Time(specificval, rsLog_Date_Time_Format);
			if (std::isnan(device_time)) {
				std::wstring msg{ dsUnknown_Date_Time_Format };
				msg.append(specificval);
				emit_parsing_exception_w(msg);
				return;
			}

			// skip; event type name
			specificval = cut_column();
			// skip; signal name
			specificval = cut_column();

			log_lines.push_back(TLog_Entry{ device_time, line_counter, line });
		}
		catch (const std::exception & ex) {				
			emit_parsing_exception(ex.what());
			return;
		}
		catch (...) {
			// any parsing error causes parser to abort
			// this should not happen, when the log was correctly written and not manually modified ever since
			emit_parsing_exception(nullptr);
			return;
		}
	}	

	//As we have read the times and end of lines, no event has been created yet => no event logical clock advanced by us
	//=> by creating the events inside the following for, log-events and by-them-triggered events will have monotonically increasing logical clocks.
	std::sort(log_lines.begin(), log_lines.end(), [](const TLog_Entry& a, const TLog_Entry& b) {
		if (std::get<idxLog_Entry_Time>(a) != std::get<idxLog_Entry_Time>(b))
			return std::get<idxLog_Entry_Time>(a) < std::get<idxLog_Entry_Time>(b);

			//if there were multiple events emitted with the same device time,
			//let us emit them in their order in the log file
		else  return std::get<idxLog_Entry_Counter>(a) < std::get<idxLog_Entry_Counter>(b);
	});
	
	for (size_t i = 0; i < log_lines.size(); i++) {
		try {
			line_counter = std::get<idxLog_Entry_Counter>(log_lines[i]);	//for the error reporting

			const double device_time = std::get<idxLog_Entry_Time>(log_lines[i]);
			line = std::move(std::get<idxLog_Entry_Line>(log_lines[i]));

			// specific column (titled "info", but contains parameters or level)
			const auto info_str = cut_column();


			auto read_segment_id = [&cut_column, &emit_parsing_exception_w]()->uint64_t {
				bool ok;
				const auto str = cut_column();
				uint64_t result = wstr_2_int(str.c_str(), ok);
				if (!ok) {
					std::wstring msg{ dsError_In_Number_Format };
					msg.append(str);
					emit_parsing_exception_w(msg);
					result = std::numeric_limits<uint64_t>::max();
				}

				return result;
			};

			const uint64_t original_segment_id = read_segment_id();
			if (original_segment_id == std::numeric_limits<uint64_t>::max()) {
				return;
			}
			
			const bool can_use_filename_as_segment_id = filename_segment_id_available && (original_segment_id != scgms::Invalid_Segment_Id) && (original_segment_id != scgms::All_Segments_Id);
			const uint64_t segment_id = can_use_filename_as_segment_id ? filename_segment_id : original_segment_id;


			scgms::UDevice_Event evt{ static_cast<scgms::NDevice_Event_Code>(std::stoull(cut_column())) };

			if (evt.is_info_event())
				evt.info.set(info_str.c_str());
			else if (evt.is_level_event()) {
				bool ok;
				const double level = wstr_2_dbl(info_str.c_str(), ok);
				if (ok) {
					evt.level() = std::stod(info_str);
				} else {
					std::wstring msg{ dsError_In_Number_Format };
					msg.append(info_str);
					emit_parsing_exception_w(msg);
					return;
				}
			}
			else if (evt.is_parameters_event())
				WStr_To_Parameters(info_str, evt.parameters);

			evt.device_time() = device_time;
			evt.segment_id() = segment_id;


			// do not send shutdown event through pipes - it's a job for outer code (GUI, ..)
			// furthermore, sending this event would cancel and stop simulation - we don't want that
			if (evt.event_code() == scgms::NDevice_Event_Code::Shut_Down)
				continue;

			bool device_id_ok, signal_id_ok;
			evt.device_id() = WString_To_GUID(cut_column(), device_id_ok);
			evt.signal_id() = WString_To_GUID(cut_column(), signal_id_ok);
			if (!device_id_ok || !signal_id_ok) {
				emit_parsing_exception_w(dsInvalid_GUID);
				return;
			}


			if (Send(evt) != S_OK)
				return;
		}
		catch (const std::exception & ex) {
			emit_parsing_exception(ex.what());
			return;
		}
		catch (...) {
			// any parsing error causes parser to abort
			// this should not happen, when the log was correctly written and not manually modified ever since
			emit_parsing_exception(nullptr);
			return;
		}
	}

}

void CLog_Replay_Filter::Open_Logs() {

	if (Is_Directory(mLog_Filename_Or_Dirpath))
	{
		auto dir = List_Directory(mLog_Filename_Or_Dirpath);
		for (const auto& path : dir)
		{
			if (Is_Regular_File_Or_Symlink(path))
				Replay_Log(path);
		}
	}
	else
		Replay_Log(mLog_Filename_Or_Dirpath);

	//issue shutdown after the last log, if we were not asked to ignore it
	if (mEmit_Shutdown) {
		scgms::UDevice_Event shutdown_evt{ scgms::NDevice_Event_Code::Shut_Down };
		Send(shutdown_evt);
	}
}

HRESULT IfaceCalling CLog_Replay_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mEmit_Shutdown = configuration.Read_Bool(rsEmit_Shutdown_Msg, mEmit_Shutdown);
	mInterpret_Filename_As_Segment_Id = configuration.Read_Bool(rsInterpret_Filename_As_Segment_Id, mInterpret_Filename_As_Segment_Id);
	mLog_Filename_Or_Dirpath = configuration.Read_String(rsLog_Output_File);

	mLog_Replay_Thread = std::make_unique<std::thread>(&CLog_Replay_Filter::Open_Logs, this);
	return S_OK;
}


HRESULT CLog_Replay_Filter::Do_Execute(scgms::UDevice_Event event) {
	return Send(event);
}

void CLog_Replay_Filter::WStr_To_Parameters(const std::wstring& src, scgms::SModel_Parameter_Vector& target)
{
	std::vector<double> params;

	size_t pos = 0, pos2 = 0;

	// note that we don't use parameter names when parsing - we assume the log was correctly written, so the parameter count and order
	// matches the correct count and order, which is safe to assume, if the log file wasn't modified after being written by logger filter

	while ((pos = src.find(L"=", pos + 1)) != std::string::npos) {
		pos2 = src.find(L", ", pos2 + 1); // this will produce npos in last iteration, but that's fine, we want to cut the rest of string anyways
		params.push_back(std::stod(src.substr(pos + 1, pos - pos2 - 1)));
	}

	target.set(params);
}
