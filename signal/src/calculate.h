#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "time_segment.h"

#include <memory>
#include <set>

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
	GUID mSignal_Id = Invalid_GUID;
	double mPrediction_Window = 0.0;
protected:
	std::map<int64_t, std::unique_ptr<CTime_Segment>> mSegments;
	double mLast_Pending_time;	//it is faster to query it rather than to query the vector correctly
	std::set<double> mPending_Times;
	glucose::SModel_Parameter_Vector mWorking_Parameters;
	std::vector<glucose::SModel_Parameter_Vector> mParameter_Hints;

	void Add_Level(const int64_t segment_id, const GUID &signal_id, const double level, const double time_stamp);
	void Emit_Levels_At_Pending_Times(const int64_t segment_id);
	void Reset_Segment(const int64_t segment_id);
	void Set_Parameters(const int64_t segment_id, glucose::SModel_Parameter_Vector parameters);
	void Add_Parameters_Hint(glucose::SModel_Parameter_Vector parameters);
public:
	CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
	virtual ~CCalculate_Filter() {};

	virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;
};

#pragma warning( pop )
