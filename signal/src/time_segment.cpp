/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8
 * 301 00, Pilsen
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "../../../common/rtl/UILib.h"

#include "time_segment.h"
#include "descriptor.h"

#include <cmath>

CTime_Segment::CTime_Segment(const int64_t segment_id, const GUID &calculated_signal_id, scgms::SModel_Parameter_Vector &working_parameters, const double prediction_window, scgms::SFilter output)
	: mOutput(output), mCalculated_Signal_Id(calculated_signal_id), mSegment_id(segment_id), mPrediction_Window(prediction_window) {
	Clear_Data();

	mWorking_Parameters.set(working_parameters);

	scgms::TModel_Descriptor desc = scgms::Null_Model_Descriptor;
	const bool result = scgms::get_model_descriptor_by_signal_id(calculated_signal_id, desc);

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

scgms::SSignal CTime_Segment::Get_Signal_Internal(const GUID &signal_id) {
	auto itr = mSignals.find(signal_id);
	if (itr != mSignals.end()) {
		return itr->second;
	} else {
		//we have to create signal's instance for this object
		scgms::STime_Segment shared_this = refcnt::make_shared_reference_ext<scgms::STime_Segment, scgms::ITime_Segment>(this, true);
		scgms::SSignal new_signal{shared_this, signal_id };
		if (!new_signal) return scgms::SSignal{};
		mSignals[signal_id] = new_signal;
		return new_signal;
	}
}

HRESULT IfaceCalling CTime_Segment::Get_Signal(const GUID *signal_id, scgms::ISignal **signal) {
	const auto &shared_signal = Get_Signal_Internal(*signal_id);
	if (shared_signal) {
		*signal = shared_signal.get();
		(*signal)->AddRef();
		return S_OK;
	} else
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

bool CTime_Segment::Set_Parameters(scgms::SModel_Parameter_Vector parameters) {
	return mWorking_Parameters.set(parameters);	//make a deep copy to ensure that the shared object will not be gone unexpectedly
}

scgms::SModel_Parameter_Vector CTime_Segment::Get_Parameters() {
	return mWorking_Parameters;
}

bool CTime_Segment::Calculate(const std::vector<double> &times, std::vector<double> &levels) {
	if (mCalculated_Signal) {
		levels.resize(times.size());
		return mCalculated_Signal->Get_Continuous_Levels(mWorking_Parameters.get(), times.data(), levels.data(), levels.size(), scgms::apxNo_Derivation) == S_OK;
	} 

	return false;
}

void CTime_Segment::Emit_Levels_At_Pending_Times() {
	if (mPending_Times.empty() || !mCalculated_Signal) return;

	std::vector<double> levels(mPending_Times.size()), times{ mPending_Times.begin(), mPending_Times.end() };
	if (levels.size() != times.size()) return;	//allocation error!

	//auto params_ptr = mWorking_Parameters.operator bool() ? mWorking_Parameters.get() : nullptr;	- mWorking params are always initialized => the same effect as nullptr
	if (mCalculated_Signal->Get_Continuous_Levels(mWorking_Parameters.get(), times.data(), levels.data(), levels.size(), scgms::apxNo_Derivation) == S_OK) {
		mPending_Times.clear();

		//send non-NaN values
		for (size_t i = 0; i < levels.size(); i++) {
			if (!std::isnan(levels[i])) {
				scgms::UDevice_Event calcEvt{ scgms::NDevice_Event_Code::Level };
				calcEvt.device_time() = times[i];
				calcEvt.level() = levels[i];
				calcEvt.device_id() = calculate::Calculate_Filter_GUID;
				calcEvt.signal_id() = mCalculated_Signal_Id;
				calcEvt.segment_id() = mSegment_id;
				scgms::IDevice_Event *raw_calcEvt = calcEvt.get();
				calcEvt.release();
				if (mOutput->Execute(raw_calcEvt) == S_OK)
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
