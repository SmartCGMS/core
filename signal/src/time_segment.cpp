#pragma once

#include "time_segment.h"


CTime_Segment::CTime_Segment(const GUID &calculated_signal_id)  {
	mCalculated_Signal = Get_Signal_Internal(calculated_signal_id);	//creates the calculated signal
}

glucose::SSignal CTime_Segment::Get_Signal_Internal(const GUID &signal_id) {
	auto itr = mSignals.find(signal_id);
	if (itr != mSignals.end()) {
		return itr->second;
		
	}
	else {
		//we have to create signal's instance for this object
		glucose::STime_Segment shared_this = refcnt::make_shared_reference_ext<glucose::STime_Segment, glucose::ITime_Segment>(this, true);
		glucose::SSignal new_signal{shared_this, signal_id };
		if (!new_signal) return glucose::SSignal{};
		mSignals[signal_id] = new_signal;
		return new_signal;
	}
}

HRESULT IfaceCalling CTime_Segment::Get_Signal(const GUID *signal_id, glucose::ISignal **signal) {
	const auto &shared_signal = Get_Signal_Internal(*signal_id);
	if (shared_signal) {
		*signal = shared_signal.get();
		(*signal)->AddRef();
	}
	else
		return E_FAIL;

}

bool CTime_Segment::Add_Level(const GUID &signal_id, const double level, const double time_stamp) {
	auto signal = Get_Signal_Internal(signal_id);
	if (signal) {
		signal->Add_Levels(&time_stamp, &level, 1);
		return true;
	}
	else
		return false;
}

bool CTime_Segment::Calculate(glucose::SModel_Parameter_Vector parameters, const std::vector<double> &times, std::vector<double> &levels) {
	if (mCalculated_Signal) {
		levels.resize(times.size());
		return mCalculated_Signal->Get_Continuous_Levels(parameters.get(), times.data(), levels.data(), levels.size(), glucose::apxNo_Derivation) == S_OK;
	}
}