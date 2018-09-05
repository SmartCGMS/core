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
#include <list>
#include <map>
#include <mutex>
#include <condition_variable>

#include "Compute_Holder.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for computing model parameters
 */
class CCompute_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// amount of levels to trigger automatic recalculation
		int64_t mRecalcLevelsCount;
		// calculate at segment end?
		bool mRecalcSegmentEnd;
		// trigger solver on calibration?
		bool mRecalcOnCalibration;
		// hold incoming messages during solving?
		bool mHold_During_Solve;
		// total acquired level count
		int64_t mTotalLevelCount;

		// how many levels (in general) there were at the time of last recalculation?
		int64_t mLastCalculationLevelsCount;

		// flag for scheduling recalculation (due to segment stop marker, levels count, ..)
		bool mCalculationScheduled;
		// flag for forcing recalculation (user input)
		bool mCalculationForced;
        // flag for forcing parameters reset
        bool mParamsResetForced;
		// flag for suspending solver starting
		bool mSuspended;
		// always request recalculate all segments when new parameters are calculated
		bool mRecalc_With_Every_Params;

		// scheduler thread
		std::unique_ptr<std::thread> mSchedulerThread;

		// scheduler mutex
		std::mutex mScheduleMtx;
		// scheduler condition variable
		std::condition_variable mScheduleCv;

		// compute holder for this filter
		std::unique_ptr<CCompute_Holder> mComputeHolder;

		// internal indicator to terminate scheduler thread
		bool mRunning;

		// thread function
		void Run_Main();
		// thread function for scheduling recalculations
		void Run_Scheduler();

	public:
		CCompute_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);

		virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;
};

#pragma warning( pop )
