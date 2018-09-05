/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "log_replay.h"
#include "log.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"

#include "log.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <ctime>
#include <vector>
#include <algorithm> 
#include <cctype>
#include <codecvt>

CLog_Replay_Filter::CLog_Replay_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }
{
}

void CLog_Replay_Filter::Run_Main()
{
	std::wstring line;
	std::wstring column;

	// read header and validate
	// NOTE: this assumes identical header (as the one used when generating log); maybe we could consider adaptive log parsing later
	if (!std::getline(mLog, line) || line != dsLog_Header)
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
	while (std::getline(mLog, line))
	{
		try
		{			
			// skip; logical time is not modifiable, and there's not a point in loading it anyways
			cut_column();
			// device time is parsed as-is using the same format as used when saving
			const double device_time = Local_Time_WStr_To_Rat_Time(cut_column(), rsLog_Date_Time_Format);
			// skip; event type name
			cut_column();
			// skip; signal name
			cut_column();

			// specific column (titled "info", but contains parameters or level)
			auto specificval = cut_column();

			const size_t segment_id = std::stoull(cut_column());

			glucose::UDevice_Event evt{ static_cast<glucose::NDevice_Event_Code>(std::stoull(cut_column())) };

			if (evt.is_info_event())
				evt.info.set(specificval.c_str());
			else if (evt.is_level_event())
				evt.level = std::stod(specificval);
			else if (evt.is_parameters_event())
				WStr_To_Parameters(specificval, evt.parameters);

			evt.device_time = device_time;
			evt.segment_id = segment_id;
			

			// do not send shutdown event through pipes - it's a job for outer code (GUI, ..)
			// furthermore, sending this event would cancel and stop simulation - we don't want that
			if (evt.event_code == glucose::NDevice_Event_Code::Shut_Down)
				break;

			evt.device_id = WString_To_GUID(cut_column());
			evt.signal_id = WString_To_GUID(cut_column());

			if (!mOutput.Send(evt))
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

bool CLog_Replay_Filter::Open_Log(glucose::SFilter_Parameters configuration)
{
	bool result = false;
	std::wstring log_file_name = configuration.Read_String(rsLog_Output_File);
	// do we have input file name?
	if (!log_file_name.empty())
	{
		// try to open file for reading
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
		mLog.open(converterX.to_bytes(log_file_name));

		result = mLog.is_open();
		// set decimal point separator
		if (result)
			mLog.imbue(std::locale(std::cout.getloc(), new logger::DecimalSeparator<char>('.')));
	}

	return result;
}

HRESULT CLog_Replay_Filter::Run(refcnt::IVector_Container<glucose::TFilter_Parameter>* const configuration) {

	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, refcnt::IVector_Container<glucose::TFilter_Parameter>>(configuration, true);
	const bool log_opened = Open_Log(shared_configuration);

	if (log_opened)
		mLog_Replay_Thread = std::make_unique<std::thread>(&CLog_Replay_Filter::Run_Main, this);

	// main loop in log reader main thread - we still need to be able to pass events from input pipe to output, althought we are the source of messages (separate thread)
	// the reason is the same as in any other "pure-input" filter like db reader - simulation commands comes through input pipe
	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
		if (!mOutput.Send(evt))
			break;
	}

	if (log_opened)
	{
		// this should also end log replay thread
		mLog.close();
		if (mLog_Replay_Thread->joinable())
			mLog_Replay_Thread->join();
	}

	return S_OK;
}

void CLog_Replay_Filter::WStr_To_Parameters(const std::wstring& src, glucose::SModel_Parameter_Vector& target)
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
