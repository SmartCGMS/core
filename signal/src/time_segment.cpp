#pragma once

#include "time_segment.h"

HRESULT IfaceCalling CTime_Segment::Get_Signal(const GUID *signal_id, glucose::ISignal **signal)  {
	auto itr = mSignals.find(*signal_id);
	if (itr != mSignals.end()) {
		*signal = (*itr).second.get();
		(*signal)->AddRef();
		return S_OK;
	}

	// prefer calculated signal, fall back to measured signal
	if (imported::create_calculated_signal(signal_id, this, signal) != S_OK)
	{
		if (imported::create_measured_signal(signal_id, this, signal) != S_OK)
			return E_NOTIMPL;
	}
	mSignals[*signal_id] = refcnt::make_shared_reference_ext<glucose::SSignal, glucose::ISignal>(*signal, true);  // true due to creating "clone" of pointer with custom reference counter

	return S_OK;

}