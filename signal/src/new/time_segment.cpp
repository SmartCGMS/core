#pragma once

#include "time_segment.h"

HRESULT IfaceCalling CTime_Segment::Get_Signal(const GUID *signal_id, glucose::ISignal **signal)  {
	auto itr = mSignals.find(*signal_id);
	if (itr != mSignals.end()) {
		//instance of the signal alread exists in this segment
		*signal = (*itr).second.get();		
	}
	else {
		//we have to create signal's instance for this object
		glucose::STime_Segment shared_this = refcnt::make_shared_reference_ext<glucose::STime_Segment, glucose::ITime_Segment>(this, true);
		glucose::SSignal new_signal{shared_this, *signal_id };
		if (!new_signal) return E_FAIL;
		mSignals[*signal_id] = new_signal;
	}
		

	(*signal)->AddRef();
	
	return S_OK;

}