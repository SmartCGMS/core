#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for calculating signals from incoming parameters
 */
class CCalculate_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		// calculated signal ID
		GUID mSignalId;

		// calculate past values when first parameter set came
		bool mCalc_Past_With_First_Params;

		// main method
		void Run_Main();

	public:
		CCalculate_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
};

#pragma warning( pop )
