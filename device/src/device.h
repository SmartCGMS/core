#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <memory>
#include <thread>

/*
 * Filter class for computing model parameters
 */
class CDevice_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		int64_t mLogicalTime;

	public:
		CDevice_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);
		virtual ~CDevice_Filter();

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;

		/* TODO: more values - sensor time, etc. */
		virtual void Signal_CGMS(float value);
};

extern "C" void signal_cgms(float value);
