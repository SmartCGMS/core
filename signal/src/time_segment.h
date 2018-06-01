#pragma once

#include "..\..\..\common\rtl\DeviceLib.h"

#include <map>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance



class CTime_Segment : public virtual glucose::ITime_Segment, public virtual refcnt::CReferenced {
protected:
	std::map<GUID, glucose::SSignal> mSignals;
public:
	virtual ~CTime_Segment() {};

	virtual HRESULT IfaceCalling Get_Signal(const GUID *signal_id, glucose::ISignal **signal) override;
};

#pragma warning( pop )