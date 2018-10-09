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
#include "../../../common/rtl/DbLib.h"

#include <memory>
#include <thread>
#include <vector>

// helper struct for storing signal generation parameters
struct TGenerator_Signal_Parameters
{
	double offset;
	double amplitude;
	double period;

	double samplingPeriod;
};

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Class that generates sinus/cosinus functions as IG/BG signals
 */
class CSinCos_Generator : public virtual glucose::IFilter, public virtual refcnt::CReferenced {
	protected:
		glucose::SFilter_Pipe mInput;
		glucose::SFilter_Pipe mOutput;

		// do we need to send shutdown after last value?
		bool mShutdownAfterLast = false;

		std::unique_ptr<std::thread> mGenerator_Thread;
		std::atomic<bool> mExit_Flag;

		// generator parameters for IG
		TGenerator_Signal_Parameters mIG_Params;
		// generator parameters for BG
		TGenerator_Signal_Parameters mBG_Params;
		// total time to be generated
		double mTotal_Time;

	protected:
		void Run_Generator();
		bool Configure(glucose::SFilter_Parameters configuration);

		bool Emit_Segment_Marker(uint64_t segment_id, bool start);
		bool Emit_Signal_Level(GUID signal_id, double time, double level, uint64_t segment_id);
		bool Emit_Shut_Down();

		void Start_Generator();
		void Terminate_Generator();

	public:
		CSinCos_Generator(glucose::SFilter_Pipe in_pipe, glucose::SFilter_Pipe out_pipe);
		virtual ~CSinCos_Generator() {};

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;
		virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration *configuration) override;
};

#pragma warning( pop )

