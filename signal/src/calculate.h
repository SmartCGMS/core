#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "Segment_Holder.h"

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
	glucose::SFilter_Pipe mInput;
	glucose::SFilter_Pipe mOutput;
protected:
	// calculated signal ID
	GUID mSignalId;
	bool mRecalculate_Past_On_Params;
	bool mRecalculate_Past_On_Segment_Stop;
public:
	CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
	virtual ~CCalculate_Filter() {};

	virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;


protected:
		

		// calculate past values when first parameter set came
		bool mCalc_Past_With_First_Params;
	protected:
		std::vector<double> mPending_Times;		//times, for which we have not calculated the levels yet
	protected:
		// main method
		void Run_Main();

};

#pragma warning( pop )
