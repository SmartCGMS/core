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

#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <memory>
#include <thread>
#include <fstream>
#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for loading previously stored log file and "replay" it through pipe
 */
class CLog_Replay_Filter : public glucose::IFilter, public virtual refcnt::CReferenced {
	protected:
		glucose::SFilter_Pipe_Reader mInput;
		glucose::SFilter_Pipe_Writer mOutput;
		std::wifstream mLog;
		bool mIgnore_Shutdown = false;
		std::wstring mLog_Filename;
		std::unique_ptr<std::thread> mLog_Replay_Thread;

	protected:
		// thread method
		void Log_Replay();

		// opens log for reading, returns true if success, false if failed
		bool Open_Log(const std::wstring &log_filename);
		// converts string to parameters vector; note that this method have no knowledge of models at all (does not validate parameter count, ..)
		void WStr_To_Parameters(const std::wstring& src, glucose::SModel_Parameter_Vector& target);

	public:
		CLog_Replay_Filter(glucose::SFilter_Pipe_Reader inpipe, glucose::SFilter_Pipe_Writer outpipe);
		virtual ~CLog_Replay_Filter() {};

		virtual HRESULT IfaceCalling Configure(glucose::IFilter_Configuration* configuration) override final;
		virtual HRESULT IfaceCalling Execute() override final;
};

#pragma warning( pop )
