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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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

#include "fast_signal_error.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/utils/math_utils.h"
#include "../../../common/utils/string_utils.h"

#include <cmath>
#include <fstream>
#include <numeric>
#include <type_traits>


namespace fast_signal_metrics {


	CAvg_SD::CAvg_SD(double& levels_counter) : mLevels_Counter(levels_counter) {
		//
	};


	double CAvg_SD::Avg_Divisor() {
		return mLevels_Counter > 1.5 ? mLevels_Counter - 1.5 + 1.0 / (8.0 * (mLevels_Counter - 1.0)) : 1.0;
	}

	void CAvg_SD::Update_Counters(const double difference) {
		const double frozen_counter = mLevels_Counter;

		mLevels_Counter += 1.0;
		mAccumulator += difference;

		const double delta = difference - (mAccumulator / Avg_Divisor());
		mVariance += delta * delta * frozen_counter / mLevels_Counter;
	}

	double CAvg_SD::Calculate_Metric() {
		const double divisor = Avg_Divisor();
		const double avg = mAccumulator / divisor;

		if (mLevels_Counter > 1.0) {
			return avg + std::sqrt(mVariance / divisor);
		}
		else
			return avg; //zero variance
	}

	void CAvg_SD::Clear_Counters() {
		mAccumulator = 0.0;
		mVariance = 0.0;
		mLevels_Counter = 0.0;
	}


	const GUID& CAvg_SD::Metric_ID() const {
		return mMetric_ID;
	}

	CAvg::CAvg(double& levels_counter) : mLevels_Counter(levels_counter) {
		//
	}

	void CAvg::Update_Counters(const double difference) {
		mAccumulator += difference;
		mLevels_Counter += 1.0;
	}

	double CAvg::Calculate_Metric() {
		return mAccumulator / mLevels_Counter;
	}


	void CAvg::Clear_Counters() {
		mAccumulator = 0.0;
		mLevels_Counter = 0.0;
	}

	const GUID& CAvg::Metric_ID() const {
		return mMetric_ID;
	}

}


CFast_Signal_Error::CFast_Signal_Error(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CFast_Signal_Error::~CFast_Signal_Error() {
	if (mPromised_Metric) {
		if (mLevels_Counter >= static_cast<double>(mLevels_Required)) {
			double metric = mCalculate_Metric();
			if (mPrefer_More_Levels)
				metric /= mLevels_Counter;
			*mPromised_Metric = metric;
		} else
			*mPromised_Metric = std::numeric_limits<double>::quiet_NaN();
	}
}

HRESULT CFast_Signal_Error::Do_Execute(scgms::UDevice_Event event) {
	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Level:
			{
				if (event.signal_id() == mReference_Signal_ID) Update_Signal_Info(event.level(), event.device_time(), true);
				else if (event.signal_id() == mError_Signal_ID) Update_Signal_Info(event.level(), event.device_time(), false);
			
				break;
			}

		case scgms::NDevice_Event_Code::Warm_Reset:
			{
				mClear_Counters();
				Clear_Signal_Info();
				mNew_Data_Logical_Clock++;
				break;
			}

		default:
			break;
	}

	return mOutput.Send(event);
}

HRESULT CFast_Signal_Error::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mReference_Signal_ID = configuration.Read_GUID(rsReference_Signal, Invalid_GUID);
	mError_Signal_ID = configuration.Read_GUID(rsError_Signal, Invalid_GUID);
	
	if (Is_Invalid_GUID(mReference_Signal_ID, mError_Signal_ID))
		return E_INVALIDARG;

	const GUID metric_id = configuration.Read_GUID(rsSelected_Metric);

	if (!Bind_Metric(metric_id, mAvg_SD, mAvg)) {
		error_description.push(dsUnsupported_Metric_Configuration);
		return E_INVALIDARG;
	}

	mClear_Counters();
	Clear_Signal_Info();

	mDescription = configuration.Read_String(rsDescription, true, GUID_To_WString(mReference_Signal_ID).append(L" - ").append(GUID_To_WString(mError_Signal_ID)));

	mRelative_Error = configuration.Read_Bool(rsUse_Relative_Error, mRelative_Error);
	mSquared_Diff = configuration.Read_Bool(rsUse_Squared_Diff, mSquared_Diff);
	mPrefer_More_Levels = configuration.Read_Bool(rsUse_Prefer_More_Levels, mPrefer_More_Levels);
	mLevels_Required = std::max(static_cast<int64_t>(1), configuration.Read_Int(rsMetric_Levels_Required, mLevels_Required));
	
	return S_OK;
}



HRESULT IfaceCalling CFast_Signal_Error::Get_Description(wchar_t** const desc) {
	*desc = const_cast<wchar_t*>(mDescription.c_str());
	return S_OK;
}


HRESULT IfaceCalling CFast_Signal_Error::QueryInterface(const GUID* riid, void** ppvObj) {
	if (Internal_Query_Interface<scgms::ISignal_Error_Inspection>(scgms::IID_Signal_Error_Inspection, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}


HRESULT IfaceCalling CFast_Signal_Error::Promise_Metric(const uint64_t segment_id, double* const metric_value, BOOL defer_to_dtor) {

	if ((segment_id == scgms::All_Segments_Id) && (defer_to_dtor == TRUE)) {
		mPromised_Metric = metric_value;
		return S_OK;
	}
	else
		return E_INVALIDARG;
}


void CFast_Signal_Error::Clear_Signal_Info() {
	mLevels_Counter = 0.0;
	for (auto& elem : mSignals) {
		elem.device_time = elem.level = elem.slope = std::numeric_limits<double>::quiet_NaN();
	}
}

void CFast_Signal_Error::Update_Signal_Info(const double level, const double device_time, const bool reference_signal) {
	mNew_Data_Logical_Clock++;

	const size_t sig_idx = static_cast<size_t>(reference_signal);
	auto& info = mSignals[sig_idx];
	
	//update the received signal
	if (!std::isnan(info.device_time)) { //not the first level
		const double dx = device_time - info.device_time;
		if (dx > 0.0) {
			//time must advance forward only
			info.slope = (level - info.level) / dx;
		}
	}

	info.device_time = device_time;
	info.level = level;


	//and try to get the other signal to compute the difference
	const auto& other_info = mSignals[sig_idx ^ 1];
	
	if (!std::isnan(other_info.slope)) {
		const double time_distance = device_time - other_info.device_time;
		//we have slope for both signals => let's calculate the difference

		if (time_distance > 0.0) {
		
			const double other_level = other_info.level - other_info.slope * time_distance;
			double difference = level - other_level;

			if (mRelative_Error) {
				const double divisor = reference_signal ? level : other_level;
				if (divisor == 0.0)
					return;	//cannot divide by zero

				difference /= divisor;
			}

			if (mSquared_Diff)
				difference *= difference;
			else
				difference = std::fabs(difference);

			mUpdate_Counters(difference);
		}
	}
}


HRESULT IfaceCalling CFast_Signal_Error::Logical_Clock(ULONG* clock) {
	if (!clock)
		return E_INVALIDARG;

	const ULONG old_clock = *clock;
	*clock = mNew_Data_Logical_Clock;

	return old_clock < *clock ? S_OK : S_FALSE;
}

HRESULT IfaceCalling CFast_Signal_Error::Calculate_Signal_Error(const uint64_t segment_id, scgms::TSignal_Stats* absolute_error, scgms::TSignal_Stats* relative_error) {
	return E_NOTIMPL;
}
