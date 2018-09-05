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
class CLog_Replay_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		glucose::SFilter_Pipe mInput;
		glucose::SFilter_Pipe mOutput;
		std::wifstream mLog;

		std::unique_ptr<std::thread> mLog_Replay_Thread;

	protected:
		// thread method
		void Run_Main();

		// opens log for reading, returns true if success, false if failed
		bool Open_Log(glucose::SFilter_Parameters configuration);
		// converts string to parameters vector; note that this method have no knowledge of models at all (does not validate parameter count, ..)
		void WStr_To_Parameters(const std::wstring& src, glucose::SModel_Parameter_Vector& target);

	public:
		CLog_Replay_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CLog_Replay_Filter() {};

		virtual HRESULT Run(refcnt::IVector_Container<glucose::TFilter_Parameter>* const configuration) override final;
};

#pragma warning( pop )
