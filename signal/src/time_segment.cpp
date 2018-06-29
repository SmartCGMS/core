#pragma once

#include "time_segment.h"
#include "descriptor.h" 

CTime_Segment::CTime_Segment(const int64_t segment_id, const GUID &calculated_signal_id, const double prediction_window, glucose::SFilter_Pipe output) : mPrediction_Window(prediction_window), mOutput(output), mSegment_id(segment_id), mCalculated_Signal_Id(calculated_signal_id) {
	
}

glucose::SSignal CTime_Segment::Get_Signal_Internal(const GUID &signal_id) {

	if ((!mCalculated_Signal) && (signal_id == mCalculated_Signal_Id))
		mCalculated_Signal = Get_Signal_Internal(mCalculated_Signal_Id);	//creates the calculated signal

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
		return S_OK;
	}
	else
		return E_FAIL;
}

bool CTime_Segment::Add_Level(const GUID &signal_id, const double level, const double time_stamp) {
	auto signal = Get_Signal_Internal(signal_id);
	if (signal) {
		if (signal->Add_Levels(&time_stamp, &level, 1) == S_OK) {
			mPending_Times.insert(time_stamp + mPrediction_Window);
		}
		return true;
	}
	else
		return false;
}

bool CTime_Segment::Set_Parameters(glucose::SModel_Parameter_Vector parameters) {
	return mWorking_Parameters.set(parameters);	//make a deep copy to ensure that the shared object will not be gone unexpectedly
}

bool CTime_Segment::Calculate(const std::vector<double> &times, std::vector<double> &levels) {
	if (mCalculated_Signal) {
		levels.resize(times.size());
		return mCalculated_Signal->Get_Continuous_Levels(mWorking_Parameters.get(), times.data(), levels.data(), levels.size(), glucose::apxNo_Derivation) == S_OK;
	} 

	return false;
}

bool CTime_Segment::Emit_Levels_At_Pending_Times() {
	bool result = false;
	std::vector<double> levels(mPending_Times.size()), times{ mPending_Times.begin(), mPending_Times.end() };
	if (levels.size() || times.size()) return result;

	if (mCalculated_Signal->Get_Continuous_Levels(mWorking_Parameters.get(), times.data(), levels.data(), levels.size(), glucose::apxNo_Derivation) == S_OK) {
		mPending_Times.clear();

		//send non-NaN values
		for (size_t i = 0; i < levels.size(); i++) {
			if (!isnan(levels[i])) {
				glucose::UDevice_Event calcEvt{ glucose::NDevice_Event_Code::Level };
				calcEvt.device_time = times[i];
				calcEvt.level = levels[i];
				calcEvt.device_id = calculate::Calculate_Filter_GUID;
				calcEvt.signal_id = mCalculated_Signal_Id;
				calcEvt.segment_id = mSegment_id;
				mOutput.Send(calcEvt);
			}
			else
				mPending_Times.insert(times[i]);
		}

		result = true;
	}

	return result;
}

void CTime_Segment::Clear_Data() {
	mSignals.clear();
	mPending_Times.clear();
	mLast_Pending_time = std::numeric_limits<double>::quiet_NaN();
}