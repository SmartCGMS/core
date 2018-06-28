#pragma once

#include "..\..\..\common\rtl\DeviceLib.h"

#include <map>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance



class CTime_Segment : public glucose::ITime_Segment, public virtual refcnt::CReferenced {
protected:
	glucose::SSignal mCalculated_Signal;
	std::map<GUID, glucose::SSignal> mSignals;
	glucose::SSignal Get_Signal_Internal(const GUID &signal_id);
public:
	CTime_Segment(const GUID &calculated_signal_id);
	virtual ~CTime_Segment() {};

	virtual HRESULT IfaceCalling Get_Signal(const GUID *signal_id, glucose::ISignal **signal) override;

	bool Add_Level(const GUID &signal_id, const double level, const double time_stamp);
	bool Calculate(glucose::SModel_Parameter_Vector parameters, const std::vector<double> &times, std::vector<double> &levels);
};

#pragma warning( pop )