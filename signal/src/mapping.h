#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for mapping input signal GUID to another
 */
class CMapping_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// source signal ID (what signal will be mapped)
		GUID mSourceId;
		// destination signal ID (to what ID it will be mapped)
		GUID mDestinationId;

		// thread function
		void Run_Main();

	public:
		CMapping_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
};

#pragma warning( pop )
