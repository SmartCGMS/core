#include "../../../common/rtl/UILib.h"

#include "time_segment.h"
#include "descriptor.h"

#include <cmath>

CTime_Segment::CTime_Segment(const int64_t segment_id, const GUID &calculated_signal_id, const double prediction_window, glucose::SFilter_Pipe output)
	: mOutput(output), mCalculated_Signal_Id(calculated_signal_id), mSegment_id(segment_id), mPrediction_Window(prediction_window) {
	Clear_Data();

	glucose::TModel_Descriptor desc = glucose::Null_Model_Descriptor;
	const bool result = glucose::get_model_descriptor_by_signal_id(calculated_signal_id, desc);

	if (result) {
		//find the proper reference id
		//mReference_Signal_Id = Invalid_GUID;	//sanity check  - already initiliazed at the variable declaration 
		for (size_t i = 0; i < desc.number_of_calculated_signals; i++)
			if (desc.calculated_signal_ids[i] == calculated_signal_id) {
				mReference_Signal_Id = desc.reference_signal_ids[i];
				break;
			}
	}
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
		return S_OK;
	}
	else
		return E_FAIL;
}

bool CTime_Segment::Add_Level(const GUID &signal_id, const double level, const double time_stamp) {
	auto signal = Get_Signal_Internal(signal_id);
	if (signal) {
		if (signal->Add_Levels(&time_stamp, &level, 1) == S_OK) {

			auto insert_the_time = [this](const double time_to_insert) {
				if (mEmitted_Times.find(time_to_insert) == mEmitted_Times.end())
					mPending_Times.insert(time_to_insert);
			};

			insert_the_time(time_stamp + mPrediction_Window);
			if (signal_id == mReference_Signal_Id)
				insert_the_time(time_stamp);		//for the reference signal, we also request calculation at the present time
													//so that we can determine calculation errors easily and precisly with measured, 
													//not interpolated levels => the metrics filter simply stores calculated-measured
													//pair of levels with no need for another calculation nor interpolation/approximation
			
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

void CTime_Segment::Emit_Levels_At_Pending_Times() {
	std::vector<double> levels(mPending_Times.size()), times{ mPending_Times.begin(), mPending_Times.end() };
	if (levels.size() != times.size()) return;	//allocation error!

	if (mCalculated_Signal->Get_Continuous_Levels(mWorking_Parameters.get(), times.data(), levels.data(), levels.size(), glucose::apxNo_Derivation) == S_OK) {
		mPending_Times.clear();

		//send non-NaN values
		for (size_t i = 0; i < levels.size(); i++) {
			if (!std::isnan(levels[i])) {
				glucose::UDevice_Event calcEvt{ glucose::NDevice_Event_Code::Level };
				calcEvt.device_time = times[i];
				calcEvt.level = levels[i];
				calcEvt.device_id = calculate::Calculate_Filter_GUID;
				calcEvt.signal_id = mCalculated_Signal_Id;
				calcEvt.segment_id = mSegment_id;
				if (mOutput.Send(calcEvt))
					mEmitted_Times.insert(times[i]);
			}
			else
				mPending_Times.insert(times[i]);
		}		
	}
}

void CTime_Segment::Clear_Data() {
	mSignals.clear();
	mPending_Times.clear();
	mEmitted_Times.clear();
	mLast_Pending_time = std::numeric_limits<double>::quiet_NaN();
	mCalculated_Signal = Get_Signal_Internal(mCalculated_Signal_Id);	//creates the calculated signal
}