#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/SolverLib.h"

#include "error_metric_counter.h"

#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for calculating error metrics
 */
class CErrors_Filter : public glucose::IErrors_Filter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		// single static instance of errors filter
		static std::atomic<CErrors_Filter*> mInstance;

		// currently used error counter instance; TODO: in future, we need to consider more segments at once
		std::unique_ptr<CError_Metric_Counter> mErrorCounter;

		// thread function
		void Run_Main();

	public:
		CErrors_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;

		// retrieves the only instance of errors filter
		static CErrors_Filter* Get_Instance();
		// retrieves error container for given signal and error type
		HRESULT Get_Errors(const GUID& signal_id, glucose::TError_Container& target, glucose::NError_Type type);
};


#pragma warning( pop )
