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
		GUID mSource_Id = Invalid_GUID;
		// destination signal ID (to what ID it will be mapped)
		GUID mDestination_Id = Invalid_GUID;
	public:
		CMapping_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CMapping_Filter() {};

		virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;
};

#pragma warning( pop )
