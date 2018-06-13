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
 * Filter class for calculating signals from incoming parameters
 */
class CCalculate_Filter : public glucose::IFilter, public virtual refcnt::CReferenced {
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// calculated signal ID
		GUID mSignalId;

		// calculate past values when first parameter set came
		bool mCalc_Past_With_First_Params;
		double mPrediction_Window = 0.0;
	protected:
		std::vector<double> mPending_Times;		//times, for which we have not calculated the levels yet
	protected:
		// main method
		void Run_Main();

	public:
		CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);

		virtual HRESULT Run(refcnt::IVector_Container<glucose::TFilter_Parameter>* const configuration) override;
};

#pragma warning( pop )
