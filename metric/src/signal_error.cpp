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

#include "signal_error.h"


#include "../../../common/lang/dstrings.h"

constexpr unsigned char bool_2_uc(const bool b) {
	return b ? static_cast<unsigned char>(1) : static_cast<unsigned char>(0);
}


CSignal_Error::CSignal_Error(glucose::IFilter *output) : CBase_Filter(output) {
	//
}

CSignal_Error::~CSignal_Error() {
	if (mPromised_Metric)
		*mPromised_Metric = Calculate_Metric();
}

HRESULT IfaceCalling CSignal_Error::QueryInterface(const GUID*  riid, void ** ppvObj) {

	if (Internal_Query_Interface<glucose::ISignal_Error>(glucose::IID_Signal_Error, *riid, ppvObj)) return S_OK;	

	return E_NOINTERFACE;
}


HRESULT CSignal_Error::Do_Execute(glucose::UDevice_Event event) {
	glucose::TDevice_Event *raw_event;

	auto add_level = [&raw_event, this](glucose::SSignal &signal) {
		std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
		mNew_Data_Available |= signal->Add_Levels(&raw_event->device_time, &raw_event->level, 1) == S_OK;
	};

	if (event->Raw(&raw_event) == S_OK) 
		if (raw_event->event_code == glucose::NDevice_Event_Code::Level)
			if (raw_event->event_code == glucose::NDevice_Event_Code::Level) add_level(mReference_Signal);
				else if (raw_event->signal_id == mError_Signal_ID) add_level(mError_Signal);			

	return Send(event);
}

HRESULT CSignal_Error::Do_Configure(glucose::SFilter_Configuration configuration) {
	mReference_Signal_ID = configuration.Read_GUID(rsReference_Signal, Invalid_GUID);
	mError_Signal_ID = configuration.Read_GUID(rsError_Signal, Invalid_GUID);
	if ((mReference_Signal_ID == Invalid_GUID) || (mError_Signal_ID == Invalid_GUID)) return E_INVALIDARG;

	const glucose::TMetric_Parameters metric_parameters{ configuration.Read_GUID(rsSelected_Metric),
		bool_2_uc(configuration.Read_Bool(rsUse_Relative_Error)),
		bool_2_uc(configuration.Read_Bool(rsUse_Squared_Diff)),
		bool_2_uc(configuration.Read_Bool(rsUse_Prefer_More_Levels)),
		configuration.Read_Double(dsMetric_Threshold)
	};
	
	mMetric = glucose::SMetric{ metric_parameters };	

	return S_OK;
}

	
bool CSignal_Error::Prepare_Levels(std::vector<double> &times, std::vector<double> &reference, std::vector<double> &error) {
	size_t reference_count = 0;
	if (mReference_Signal->Get_Discrete_Bounds(nullptr, nullptr, &reference_count)) {
		if (reference_count > 0) {

			times.resize(reference_count);
			reference.resize(reference_count);

			size_t filled;
			if (mReference_Signal->Get_Discrete_Levels(times.data(), reference.data(), reference_count, &filled) == S_OK) {
				error.resize(reference_count);

				if (mError_Signal->Get_Continuous_Levels(nullptr, times.data(), error.data(), filled, glucose::apxNo_Derivation) == S_OK) 
					return true;				
			}

		}
	}
	return false;
}

double CSignal_Error::Calculate_Metric() {
	double result = std::numeric_limits<double>::quiet_NaN();
	std::vector<double> reference_times;
	std::vector<double> reference_levels;
	std::vector<double> error_levels;

	std::lock_guard<std::mutex> lock{ mSeries_Gaurd };

	//1. calculate the difference against the exact reference levels => get their times
	if (Prepare_Levels(reference_times, reference_levels, error_levels)) {
		if (mMetric->Reset() == S_OK) {			
			if (mMetric->Accumulate(reference_times.data(), reference_levels.data(), error_levels.data(), reference_times.size()) == S_OK) {
				size_t levels_acquired = 0;
				if (mMetric->Calculate(&result, &levels_acquired, 0) == S_OK)
					if (levels_acquired == 0) result = std::numeric_limits<double>::quiet_NaN();
			}
		}
	}
	
	
	return result;
}

HRESULT IfaceCalling CSignal_Error::Promise_Metric(double* const metric_value, bool defer_to_dtor) {
	if (!defer_to_dtor) {
		*metric_value = Calculate_Metric();
		return isnan(*metric_value) ? S_FALSE : S_OK;
	}
	else {
		mPromised_Metric = metric_value;
		return S_OK;
	}
}

HRESULT IfaceCalling CSignal_Error::Peek_New_Data_Available() {
	HRESULT rc = mNew_Data_Available ? S_OK : S_FALSE;
	mNew_Data_Available = false;
	return rc;
}

HRESULT IfaceCalling CSignal_Error::Calculate_Signal_Error(const glucose::NError_Type error_type, glucose::TSignal_Error &signal_error) {
	std::vector<double> abs_differences;
	std::vector<double> reference_levels;
	std::vector<double> error_levels;

	std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
	if (Prepare_Levels(abs_differences, reference_levels, error_levels)) {

		//1. calculate sum and count
		signal_error.count = 0;
		for (size_t i = 0; i < abs_differences.size(); i++) {
			if (!isnan(reference_levels[i]) && !isnan(error_levels[i])) {
				//both levels are not nan, so we can calcualte the error here
				abs_differences[i] = fabs(reference_levels[i] - error_levels[i]);

				signal_error.sum += abs_differences[i];
				signal_error.count++;
			}
		}
		//2. test the count and if OK, calculate avg and others
		if (signal_error.count < 1) return S_FALSE;
		signal_error.avg = signal_error.sum / static_cast<double>(signal_error.count);

		//3. calculate stddev
		{
			double corrected_count = static_cast<double>(signal_error.count);
			if (corrected_count > 1.5) corrected_count -= 1.5;					//Unbiased estimation of standard deviation
				else if (corrected_count > 1.0) corrected_count -= 1.0;			//Bessel's correction

			const double corrected_avg = signal_error.avg / corrected_count;
			signal_error.stddev = 0.0;
			for (const auto & difference : abs_differences) {
				const double tmp = difference - corrected_avg;
				signal_error.stddev += tmp * tmp;
			}

			signal_error.stddev /= corrected_count;
		}

		//4. calculate the ECDF
		{			
			const double stepping = static_cast<double>(signal_error.count) /  (static_cast<double>(glucose::NECDF::max_value)+1.0);
			std::sort(abs_differences.begin(), abs_differences.end());

			size_t index = static_cast<size_t>(glucose::NECDF::min_value);
			do {				
				index++;
			} while (static_cast<glucose::NECDF>(index) != glucose::NECDF::max_value+1);
		}

		return S_OK;
	}
	else return E_FAIL;
}