#pragma once

#include "../../../common/rtl/FilterLib.h"
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
class CErrors_Filter : public glucose::IFilter, public glucose::IError_Filter_Inspection, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// currently used error counter instance; TODO: in future, we need to consider more segments at once
		std::unique_ptr<CError_Marker_Counter> mErrorCounter;
	public:
		CErrors_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CErrors_Filter() {};
		
		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj);

		virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration* configuration) override;

		// retrieves the only instance of errors filter
		virtual HRESULT IfaceCalling Get_Errors(const GUID *signal_id, const glucose::NError_Type type, glucose::TError_Markers *markers);
};


#pragma warning( pop )
