#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
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
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		// amount of levels to trigger automatic recalculation
		int64_t mRecalcLevelsCount;
		// calculate at segment end?
		bool mRecalcSegmentEnd;
		// trigger solver on calibration?
		bool mRecalcOnCalibration;
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
		CCompute_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
};

#pragma warning( pop )
