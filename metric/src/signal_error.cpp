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
#include "../../../common/rtl/UILib.h"

constexpr unsigned char bool_2_uc(const bool b) {
	return b ? static_cast<unsigned char>(1) : static_cast<unsigned char>(0);
}


CSignal_Error::CSignal_Error(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CSignal_Error::~CSignal_Error() {
	if (mPromised_Metric)
		*mPromised_Metric = Calculate_Metric();
}

HRESULT IfaceCalling CSignal_Error::QueryInterface(const GUID*  riid, void ** ppvObj) {

	if (Internal_Query_Interface<scgms::ISignal_Error_Inspection>(scgms::IID_Signal_Error_Inspection, *riid, ppvObj)) return S_OK;	

	return E_NOINTERFACE;
}


HRESULT CSignal_Error::Do_Execute(scgms::UDevice_Event event) {
	scgms::TDevice_Event *raw_event;

	auto add_level = [&raw_event, this](scgms::SSignal &signal) {
		std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
		if (signal->Add_Levels(&raw_event->device_time, &raw_event->level, 1) == S_OK)
			mNew_Data_Available = true;
	};

	if (event->Raw(&raw_event) == S_OK)
		switch (raw_event->event_code) {
		
			case scgms::NDevice_Event_Code::Level:
				if (raw_event->signal_id == mReference_Signal_ID) add_level(mReference_Signal);
					else if (raw_event->signal_id == mError_Signal_ID) add_level(mError_Signal);
			break;


			case scgms::NDevice_Event_Code::Warm_Reset:
				{
					std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
					mReference_Signal = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_BG };
					mError_Signal = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_BG };
					mNew_Data_Available = true;
				}
				break;


	}



	return Send(event);
}

HRESULT CSignal_Error::Do_Configure(scgms::SFilter_Configuration configuration) {
	mReference_Signal_ID = configuration.Read_GUID(rsReference_Signal, Invalid_GUID);
	mError_Signal_ID = configuration.Read_GUID(rsError_Signal, Invalid_GUID);
	if ((mReference_Signal_ID == Invalid_GUID) || (mError_Signal_ID == Invalid_GUID)) return E_INVALIDARG;

	mDescription = configuration.Read_String(rsDescription, GUID_To_WString(mReference_Signal_ID).append(L" - ").append(GUID_To_WString(mError_Signal_ID)));

	const scgms::TMetric_Parameters metric_parameters{ configuration.Read_GUID(rsSelected_Metric),
		bool_2_uc(configuration.Read_Bool(rsUse_Relative_Error)),
		bool_2_uc(configuration.Read_Bool(rsUse_Squared_Diff)),
		bool_2_uc(configuration.Read_Bool(rsUse_Prefer_More_Levels)),
		configuration.Read_Double(dsMetric_Threshold, 0.0)
	};
	
	mMetric = scgms::SMetric{ metric_parameters };	

	return S_OK;
}

	
bool CSignal_Error::Prepare_Levels(std::vector<double> &times, std::vector<double> &reference, std::vector<double> &error) {
	size_t reference_count = 0;
	if (SUCCEEDED(mReference_Signal->Get_Discrete_Bounds(nullptr, nullptr, &reference_count))) {
		if (reference_count > 0) {

			times.resize(reference_count);
			reference.resize(reference_count);

			size_t filled;
			if (mReference_Signal->Get_Discrete_Levels(times.data(), reference.data(), reference_count, &filled) == S_OK) {
				error.resize(reference_count);

				if (mError_Signal->Get_Continuous_Levels(nullptr, times.data(), error.data(), filled, scgms::apxNo_Derivation) == S_OK) 
					return true;				
			}

		}
		else
			return true;	//no error, but simply no data yet
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
	return mNew_Data_Available.exchange(false) ? S_OK : S_FALSE;
}

HRESULT IfaceCalling CSignal_Error::Calculate_Signal_Error(scgms::TSignal_Error *absolute_error, scgms::TSignal_Error *relative_error) {
	if (!absolute_error || !relative_error) return E_INVALIDARG;

		//be aware that it sorts the differences vector
	auto Calculate_StdDev_And_ECDF = [](std::vector<double> &differences, scgms::TSignal_Error &signal_error) {
		//3. calculate stddev
		{
			double corrected_count = static_cast<double>(signal_error.count);
			if (corrected_count > 1.5) corrected_count -= 1.5;					//Unbiased estimation of standard deviation
				else if (corrected_count > 1.0) corrected_count -= 1.0;			//Bessel's correction

			const double corrected_avg = signal_error.avg / corrected_count;
			signal_error.stddev = 0.0;
			for (const auto & difference : differences) {
				const double tmp = difference - corrected_avg;
				signal_error.stddev += tmp * tmp;
			}

			signal_error.stddev /= corrected_count;
		}

		//4. calculate ECDF
		std::sort(differences.begin(), differences.end());

		//fill min and max precisely as we will be rounding for the other values
		signal_error.ecdf[0] = differences[0];
		signal_error.ecdf[static_cast<size_t>(scgms::NECDF::max_value)] = differences[differences.size() - 1];

		const double stepping = static_cast<double>(signal_error.count-1) / (static_cast<double>(scgms::NECDF::max_value) + 1.0);
		const size_t ECDF_offset = static_cast<size_t>(scgms::NECDF::min_value);
		for (size_t i = 1; i < static_cast<size_t>(scgms::NECDF::max_value) - ECDF_offset; i++)
			signal_error.ecdf[i + ECDF_offset] = differences[static_cast<size_t>(round(static_cast<double>(i)*stepping))];
	};

	auto Set_Error_To_No_Data = [](scgms::TSignal_Error &signal_error) {
		signal_error.count = 0;
		signal_error.sum = 0.0;
		signal_error.avg = std::numeric_limits<double>::quiet_NaN();
		signal_error.stddev = std::numeric_limits<double>::quiet_NaN();

		const size_t ECDF_offset = static_cast<size_t>(scgms::NECDF::min_value);
		for (size_t i = 0; i <= static_cast<size_t>(scgms::NECDF::max_value) - ECDF_offset; i++)
			signal_error.ecdf[ECDF_offset + i] = std::numeric_limits<double>::quiet_NaN();
	};


	std::vector<double> times;
	std::vector<double> reference_levels;
	std::vector<double> error_levels;

	std::lock_guard<std::mutex> lock{ mSeries_Gaurd };
	if (Prepare_Levels(times, reference_levels, error_levels)) {

		//let's reuse the already allocated memory
		decltype(error_levels) &absolute_differences = error_levels;			//has to be error level not to overwrite reference too soon
		decltype(reference_levels) &relative_differences = reference_levels;		

		//1. calculate sum and count
		absolute_error->count = 0;
		relative_error->count = 0;
		for (size_t i = 0; i < reference_levels.size(); i++) {
			if (!isnan(reference_levels[i]) && !isnan(error_levels[i])) {
				//both levels are not nan, so we can calcualte the error here
				absolute_differences[absolute_error->count] = fabs(reference_levels[i] - error_levels[i]);
				absolute_error->sum += absolute_differences[absolute_error->count];				

				if (reference_levels[i] != 0.0) {
					relative_differences[relative_error->count] = absolute_differences[absolute_error->count] / reference_levels[i];
					relative_error->sum += relative_differences[relative_error->count];
					relative_error->count++;
				}

				absolute_error->count++;
			}
		}
		//2. test the count and if OK, calculate avg and others
		if (absolute_error->count < 1) {
			Set_Error_To_No_Data(*absolute_error);
			Set_Error_To_No_Data(*relative_error);
			return S_FALSE;
		}
		absolute_error->avg = absolute_error->sum / static_cast<double>(absolute_error->count);
		Calculate_StdDev_And_ECDF(absolute_differences, *absolute_error);

		if (relative_error->count > 0) {
			relative_error->avg = relative_error->sum / static_cast<double>(relative_error->count);
			Calculate_StdDev_And_ECDF(relative_differences, *relative_error);
			} else Set_Error_To_No_Data(*relative_error);		
	

		return S_OK;
	}
	else return E_FAIL;
}

HRESULT IfaceCalling CSignal_Error::Get_Description(wchar_t** const desc) {
	*desc = const_cast<wchar_t*>(mDescription.c_str());
	return S_OK;
}