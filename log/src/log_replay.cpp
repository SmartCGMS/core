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
#include <set>
#include <cwctype>

CLog_Replay_Filter::CLog_Replay_Filter(scgms::IFilter* output) : CBase_Filter(output) {
	//
}

CLog_Replay_Filter::~CLog_Replay_Filter() {
	mShutdown_Received = true; //ensure that the shutdown flag is set,
							   //because we are shutting down, regardless of the shutdown messsage received
							   //e.g.; on aborting from a failed configuration of a successive thread

	if (mLog_Replay_Thread)
		if (mLog_Replay_Thread->joinable())
			mLog_Replay_Thread->join();
}


void CLog_Replay_Filter::Replay_Log(const filesystem::path& log_filename, uint64_t filename_segment_id) {
	if (log_filename.empty()) return;

	std::wifstream log{ log_filename };
	if (!log.is_open()) {
		Emit_Info(scgms::NDevice_Event_Code::Error, dsCannot_Open_File + log_filename.wstring());
		return;
	}

	if (mInterpret_Filename_As_Segment_Id)
		Emit_Info(scgms::NDevice_Event_Code::Information, dsProcessing_File + log_filename.wstring(), filename_segment_id);

	//unused keeps static analysis happy about creating an unnamed object
	auto unused = log.imbue(std::locale(std::cout.getloc(), new CDecimal_Separator<char>{ '.' })); //locale takes owner ship of dec_sep

	std::wstring line;
	std::wstring column;
	size_t line_counter = 0;

	// read header and validate
	// NOTE: this assumes identical header (as the one used when generating log); maybe we could consider adaptive log parsing later	
	if (!std::getline(log, line) || (trim(line).find(dsLog_Header) != 0)) {	//we permit other (possibly future-version) fields after our header
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

		auto pos = line.find(L';');
		if (pos != std::string::npos) {
			retstr = line.substr(0, pos);
			line.erase(0, pos + 1/*len of ';'*/);
		}
		else retstr = line;

		return trim(retstr);
	};

	auto emit_parsing_exception_w = [this, &log_filename, line_counter](const std::wstring& what) {

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

	auto read_segment_id = [&cut_column, &emit_parsing_exception_w]()->uint64_t {
		bool ok;
		const auto str = cut_column();
		uint64_t result = wstr_2_uint(str.c_str(), ok);
		if (!ok) {
			std::wstring msg{ dsError_In_Number_Format };
			msg.append(str);
			emit_parsing_exception_w(msg);
			result = std::numeric_limits<uint64_t>::max();
		}

		return result;
	};


	std::vector<TLog_Entry> log_lines;	//first, we fetch the lines, and then we parse them
				//we do so, to ensure that all the events are time sorted and and events't logical clock are monotonically increasing

	std::map<uint64_t, uint64_t> segment_id_map;

	// read all lines from log file
	while (std::getline(log, line) && !mShutdown_Received) {
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

			const auto info_str = std::move(cut_column());
			const uint64_t original_segment_id = read_segment_id();

			if ((original_segment_id != scgms::Invalid_Segment_Id) && (original_segment_id != scgms::All_Segments_Id))
				segment_id_map[original_segment_id] = original_segment_id;

			log_lines.push_back(TLog_Entry{ device_time, line_counter, info_str, original_segment_id, line });
		}
		catch (const std::exception& ex) {
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

	//Adjust segment ids if there were multiple segments
	if (mInterpret_Filename_As_Segment_Id) {
		if (segment_id_map.size() > 1) {
			uint64_t last_segment_id = 1;
			uint64_t segment_id_base = filename_segment_id * 1000;
			while (segment_id_base < segment_id_map.size() * 1000)
				segment_id_base *= 10;


			for (auto& segment_id : segment_id_map) {

				segment_id.second = segment_id_base + last_segment_id;
				last_segment_id++;

			}
		}
		else if (segment_id_map.size() == 1) {
			segment_id_map.begin()->second = filename_segment_id;
		}
	}
	//add the special cases
	segment_id_map[scgms::Invalid_Segment_Id] = scgms::Invalid_Segment_Id;
	segment_id_map[scgms::All_Segments_Id] = scgms::All_Segments_Id;

	for (size_t i = 0; i < log_lines.size(); i++) {
		if (mShutdown_Received) break;

		try {
			line_counter = std::get<idxLog_Entry_Counter>(log_lines[i]);	//for the error reporting

			const double device_time = std::get<idxLog_Entry_Time>(log_lines[i]);

			// specific column (titled "info", but contains parameters or level)
			const auto info_str = std::move(std::get<idxLog_Info_Line>(log_lines[i]));
			const uint64_t segment_id = segment_id_map[std::get<idxLog_Segment_Id>(log_lines[i])];
			line = std::move(std::get<idxLog_Entry_Line>(log_lines[i]));


			scgms::UDevice_Event evt{ static_cast<scgms::NDevice_Event_Code>(std::stoull(cut_column())) };

			if (evt.is_info_event())
				evt.info.set(info_str.c_str());
			else if (evt.is_level_event()) {
				bool ok;
				const double level = wstr_2_dbl(info_str.c_str(), ok);
				if (ok) {
					evt.level() = level;
				}
				else {
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


			if (!Succeeded(mOutput.Send(evt)))
				return;
		}
		catch (const std::exception& ex) {
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


bool Match_Wildcard(const std::wstring fname, const std::wstring wcard, const bool case_sensitive) {
	size_t f = 0;
	for (size_t w = 0; w < wcard.size(); w++) {
		switch (wcard[w]) {
		case L'?':
			if (f >= fname.size()) return false;
			//there is one char to eat, let's continue
			f++;
			break;


		case L'*':
			//skip everything in the filename until extension or dir separator
			while (f < fname.size()) {
				if ((fname[f] == L'.') || (fname[f] == filesystem::path::preferred_separator))
					break;

				f++;
			}
			break;


		default:
			if (f >= fname.size()) return false;
			if (case_sensitive) {
				if (wcard[w] != fname[f]) return false;
			}
			else {
				if (std::towupper(wcard[w]) != std::towupper(fname[f])) return false;
			}
			//wild card and name still matches, continue
			f++;
			break;

		}


	}

	return f < fname.size() ? false : true;	//return false if some chars in the fname were not eaten
}



std::vector<CLog_Replay_Filter::TLog_Segment_id> CLog_Replay_Filter::Enumerate_Log_Segments() {
	std::vector<TLog_Segment_id> logs_to_replay;

	const auto effective_path = mLog_Filename_Or_Dirpath.parent_path();
	const std::wstring wildcard = mLog_Filename_Or_Dirpath.wstring();
	constexpr bool case_sensitive =
#ifdef  _WIN32
		false
#else
		true
#endif   
		;


	std::error_code ec;
	if (effective_path.empty() || (!filesystem::exists(effective_path, ec) || ec))
		return logs_to_replay;


	//1. gather a list of all segments we will try to replay
	if (Is_Directory(effective_path)) {
		for (auto& path : filesystem::directory_iterator(effective_path)) {
			const bool matches_wildcard = Match_Wildcard(path.path().wstring(), wildcard, case_sensitive);

			if (matches_wildcard) {
				if (Is_Regular_File_Or_Symlink(path))
					logs_to_replay.push_back({ path, scgms::Invalid_Segment_Id });
			}
		}
	}
	else
		logs_to_replay.push_back({ mLog_Filename_Or_Dirpath, scgms::Invalid_Segment_Id });


	if (!logs_to_replay.empty()) {
		//2. determine segment_ids if asked to do so
		if (mInterpret_Filename_As_Segment_Id) {
			std::set<uint64_t> used_ids;
			uint64_t recent_id = 0;

			for (auto& log : logs_to_replay) {
				std::string name = log.file_name.stem().string();
				//let's find first digit in the filename
				char* first_char = nullptr;
				char* end_char = nullptr;
				for (size_t i = 0; i < name.size(); i++)
					if (std::isdigit(name[i])) {
						first_char = name.data() + i;
						break;
					}

				//try to convert
				if (first_char)
					log.segment_id = std::strtoull(first_char, &end_char, 10);

				size_t near_id = 0;

				while ((log.segment_id <= 0) ||	//<= just in the case that we would change max_id to a signed int
					(log.segment_id == scgms::Invalid_Segment_Id) ||
					(log.segment_id == scgms::All_Segments_Id) ||
					(used_ids.find(log.segment_id) != used_ids.end())) {

					if (used_ids.find(log.segment_id + near_id) == used_ids.end()) {
						log.segment_id += ++near_id;
					}
					else {
						log.segment_id = ++recent_id;
					}
				}

				used_ids.insert(log.segment_id);
			}

			std::sort(logs_to_replay.begin(), logs_to_replay.end(), [](auto& a, auto& b) {return a.segment_id < b.segment_id; });
		}

	}

	return logs_to_replay;
}

void CLog_Replay_Filter::Open_Logs(std::vector<CLog_Replay_Filter::TLog_Segment_id> logs_to_replay) {
	for (auto& log : logs_to_replay)
		if (!mShutdown_Received)
			Replay_Log(log.file_name, log.segment_id);

	//issue shutdown after the last log, if we were not asked to ignore it
	if (mEmit_Shutdown) {
		scgms::UDevice_Event shutdown_evt{ scgms::NDevice_Event_Code::Shut_Down };
		mOutput.Send(shutdown_evt);
	}
}

HRESULT IfaceCalling CLog_Replay_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mEmit_Shutdown = configuration.Read_Bool(rsEmit_Shutdown_Msg, mEmit_Shutdown);
	mInterpret_Filename_As_Segment_Id = configuration.Read_Bool(rsInterpret_Filename_As_Segment_Id, mInterpret_Filename_As_Segment_Id);
	mLog_Filename_Or_Dirpath = configuration.Read_File_Path(rsLog_Output_File);

	std::vector<CLog_Replay_Filter::TLog_Segment_id> logs_to_replay = Enumerate_Log_Segments();
	if (!logs_to_replay.empty())
		mLog_Replay_Thread = std::make_unique<std::thread>(&CLog_Replay_Filter::Open_Logs, this, std::move(logs_to_replay));
	else {
		error_description.push(dsCannot_Open_File + mLog_Filename_Or_Dirpath.wstring());
		return ERROR_FILE_NOT_FOUND;
	}

	return S_OK;
}


HRESULT CLog_Replay_Filter::Do_Execute(scgms::UDevice_Event event) {
	if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
		mShutdown_Received = true;
	return mOutput.Send(event);
}

void CLog_Replay_Filter::WStr_To_Parameters(const std::wstring& src, scgms::SModel_Parameter_Vector& target) {
	std::vector<double> params;

	size_t pos = 0, pos2 = 0;

	// note that we don't use parameter names when parsing - we assume the log was correctly written, so the parameter count and order
	// matches the correct count and order, which is safe to assume, if the log file wasn't modified after being written by logger filter

	while ((pos = src.find(L"=", pos + 1)) != std::string::npos) {
		pos2 = src.find(L", ", pos2 + 1); // this will produce npos in last iteration, but that's fine, we want to cut the rest of string anyway
		params.push_back(std::stod(src.substr(pos + 1, pos - pos2 - 1)));
	}

	target.set(params);
}