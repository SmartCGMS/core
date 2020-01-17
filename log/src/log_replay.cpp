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
#include <filesystem>

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

	std::wifstream log{ Narrow_WChar(log_filename.c_str()) };
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

	// read header and validate
	// NOTE: this assumes identical header (as the one used when generating log); maybe we could consider adaptive log parsing later
	if (!std::getline(log, line) || line != std::wstring(dsLog_Header))
		return;

	// cuts a single column from input line
	auto cut_column = [&line]() -> std::wstring {
		std::wstring retstr{ L"" };
		
		auto pos = line.find(rsLog_CSV_Separator);
		if (pos != std::string::npos) {
			retstr = line.substr(0, pos);
			line.erase(0, pos + wcslen(rsLog_CSV_Separator));
		}
		else retstr = line;
		
		return retstr;
	};

	// read all lines from log file
	while (std::getline(log, line))
	{
		try
		{
			// skip; logical time is not modifiable, and there's not a point in loading it anyways
			auto specificval = cut_column();
			// device time is parsed as-is using the same format as used when saving
			const double device_time = Local_Time_WStr_To_Rat_Time(cut_column(), rsLog_Date_Time_Format);
			// skip; event type name
			specificval = cut_column();
			// skip; signal name
			specificval = cut_column();

			// specific column (titled "info", but contains parameters or level)
			specificval = cut_column();

			const uint64_t original_segment_id = std::stoull(cut_column());
			const bool can_use_filename_as_segment_id = filename_segment_id_available && (original_segment_id != scgms::Invalid_Segment_Id) && (original_segment_id != scgms::All_Segments_Id);
			const uint64_t segment_id = can_use_filename_as_segment_id ? filename_segment_id : original_segment_id;
				

			scgms::UDevice_Event evt{ static_cast<scgms::NDevice_Event_Code>(std::stoull(cut_column())) };

			if (evt.is_info_event())
				evt.info.set(specificval.c_str());
			else if (evt.is_level_event())
				evt.level() = std::stod(specificval);
			else if (evt.is_parameters_event())
				WStr_To_Parameters(specificval, evt.parameters);

			evt.device_time() = device_time;
			evt.segment_id() = segment_id;


			// do not send shutdown event through pipes - it's a job for outer code (GUI, ..)
			// furthermore, sending this event would cancel and stop simulation - we don't want that
			if (evt.event_code() == scgms::NDevice_Event_Code::Shut_Down)
				continue;			

			evt.device_id() = WString_To_GUID(cut_column());
			evt.signal_id() = WString_To_GUID(cut_column());

			if (Send(evt) != S_OK)
				return;
		}
		catch (...)
		{
			// any parsing error causes parser to abort
			// this should not happen, when the log was correctly written and not manually modified ever since
			return;
		}
	}	
}

void CLog_Replay_Filter::Open_Logs() {
	if (std::filesystem::is_directory(mLog_Filename_Or_Dirpath)) {

		std::filesystem::directory_iterator dir{ mLog_Filename_Or_Dirpath };
		for (const auto& entry : dir) {
			if ((dir->is_regular_file()) || (dir->is_symlink()))
				Replay_Log(dir->path());
		}
	} else
		Replay_Log(mLog_Filename_Or_Dirpath);

	//issue shutdown after the last log, if we were not asked to ignore it
	if (mEmit_Shutdown) {
		scgms::UDevice_Event shutdown_evt{ scgms::NDevice_Event_Code::Shut_Down };
		Send(shutdown_evt);
	}
}

HRESULT IfaceCalling CLog_Replay_Filter::Do_Configure(scgms::SFilter_Configuration configuration) {
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
