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

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/FilesystemLib.h"

#include <memory>
#include <thread>
#include <fstream>
#include <vector>
#include <atomic>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

 /*
  * Filter class for loading previously stored log file and "replay" it through pipe
  */
class CLog_Replay_Filter : public virtual scgms::CBase_Filter {
protected:
	static const size_t idxLog_Entry_Time = 0;
	static const size_t idxLog_Entry_Counter = 1;
	static const size_t idxLog_Info_Line = 2;
	static const size_t idxLog_Segment_Id = 3;
	static const size_t idxLog_Entry_Line = 4;
	using TLog_Entry = std::tuple<double, size_t, std::wstring, uint64_t, std::wstring>;
protected:
	struct TLog_Segment_id {
		filesystem::path file_name;
		uint64_t segment_id;
	};
	std::vector<TLog_Segment_id> Enumerate_Log_Segments();
protected:
	bool mEmit_Shutdown = false;
	bool mInterpret_Filename_As_Segment_Id = false;
	filesystem::path mLog_Filename_Or_Dirpath;  //would prefere wildcard, but this is not covered by C++ standard and do not need that so much to implement it using regex
	std::unique_ptr<std::thread> mLog_Replay_Thread;
protected:
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
protected:
	// thread method
	std::atomic<bool> mShutdown_Received { false};
	void Replay_Log(const filesystem::path& log_filename, uint64_t filename_segment_id);

	// opens log for reading, returns true if success, false if failed
	void Open_Logs(std::vector<CLog_Replay_Filter::TLog_Segment_id> logs_to_replay);
	// converts string to parameters vector; note that this method have no knowledge of models at all (does not validate parameter count, ..)
	void WStr_To_Parameters(const std::wstring& src, scgms::SModel_Parameter_Vector& target);

public:
	CLog_Replay_Filter(scgms::IFilter* output);
	virtual ~CLog_Replay_Filter();
};

#pragma warning( pop )
