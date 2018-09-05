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
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <tbb/concurrent_queue.h>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for holding input events until the real time matches the event device time
 */
class CHold_Filter : public glucose::IFilter, public virtual refcnt::CReferenced {
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// mutex for waiting on CV
		std::mutex mHoldMtx;
		// condition variable for waiting
		std::condition_variable mHoldCv;
		// was the CV properly notified? (spurious wakeup didn't occur)
		std::atomic<size_t> mNotified;
		// the simulation offset of held values; this value increases with each simulation step
		double mSimulationOffset;
		// time wait in milliseconds; if equals 0, we hold messages according to real time
		size_t mMsWait;

		// is the hold filter still supposed to run?
		std::atomic<bool> mRunning;

		// thread for holding events
		std::unique_ptr<std::thread> mHoldThread;
		// queue of events being held
		tbb::concurrent_bounded_queue<glucose::IDevice_Event*> mQueue;

		// thread function
		void Run_Main();

		// thread function for holding inputs
		void Run_Hold();

	public:
		CHold_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);

		// method for notifying condition variable and performing simulation step
		void Simulation_Step(size_t stepcount);

		virtual HRESULT Run(refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override;
};

#pragma warning( pop )
