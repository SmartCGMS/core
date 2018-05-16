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
		tbb::concurrent_bounded_queue<glucose::SDevice_Event> mQueue;

		// thread function
		void Run_Main();

		// thread function for holding inputs
		void Run_Hold();

	public:
		CHold_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);

		// method for notifying condition variable and performing simulation step
		void Simulation_Step(size_t stepcount);

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
};

#pragma warning( pop )
