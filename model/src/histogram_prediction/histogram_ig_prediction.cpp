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

#include "histogram_ig_prediction.h"
#include "../descriptor.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/utils/math_utils.h"

#include <numeric>

namespace hist_ig_pred {
	size_t Level_To_Histogram_Index(const double level) {
		if (level >= High_Threshold) return Band_Count - 1;

		const double tmp = level - Low_Threshold;
		if (tmp < 0.0) return 0;

		return static_cast<size_t>(floor(tmp*Inv_Band_Size));
	}

	double Histogram_Index_To_Level(const size_t index) {
		if (index == 0) return Low_Threshold - Half_Band_Size;
		if (index >= Band_Count - 1) return High_Threshold + Half_Band_Size;

		return Low_Threshold + static_cast<double>(index - 1)*Band_Size + Half_Band_Size;
	}

	void Inc_Histogram(THistogram &histogram, const size_t index) {
		//we do not use OOP to conserve memory
		//=>therefore we use just one byte to represent the counters
		//as the histogram will be actually transformed to probability distribution
		//we just need to identify the peaks => halving the counters

		uint8_t &val = histogram[index];
		if (val == std::numeric_limits<std::remove_reference<decltype(histogram[0])>::type>::max()) {
			for (auto &val_to_half : histogram)
				val_to_half /= 2;
		}
		val++;
	}

	TProbability Histogram_To_Probability(const THistogram &histogram) {
		TProbability result;
		for (size_t i = 0; i < histogram.size(); i++)
			result(i) = histogram[i];

		const double max = result.maxCoeff();
		if (max != 0.0)
			result /= max;

		//result.normalize();

		return result;
	}

	TSampling_Delta Prediction_Sampling_Delta(const double horizon) {
		TSampling_Delta result;

		//30 minutes to the history, we we sample to predict
		//memory allows us to use T5th only, se we make a trick
		//and resample by 6 minutes
		result(0) = -120.0*scgms::One_Minute - horizon;
		result(1) = -90.0*scgms::One_Minute - horizon;
		result(2) = -60.0*scgms::One_Minute - horizon;
		result(3) = -30.0*scgms::One_Minute - horizon; 
		result(4) = -0.0*scgms::One_Minute - horizon;

		return result;
	}

}

CHistogram_IG_Prediction::CHistogram_IG_Prediction(scgms::IFilter *output) 
	: scgms::CBase_Filter(output, hist_ig_pred::id) {
}

CHistogram_IG_Prediction::~CHistogram_IG_Prediction() {

}

HRESULT CHistogram_IG_Prediction::Do_Execute(scgms::UDevice_Event event) {
	if ((event.event_code() == scgms::NDevice_Event_Code::Level) &&
		(event.signal_id() == scgms::signal_IG)) {

		const double dev_time = event.device_time();
		const uint64_t seg_id = event.segment_id();
		const double level = event.level();
		
		HRESULT rc = Send(event);
		if (SUCCEEDED(rc)) {			
			const double pred_level = Update_And_Predict(dev_time, level);

			if (std::isnormal(pred_level)) {
				scgms::UDevice_Event pred{ scgms::NDevice_Event_Code::Level };
				pred.device_id() = hist_ig_pred::id;
				pred.level() = pred_level;
				pred.device_time() = dev_time + mDt;
				pred.signal_id() = hist_ig_pred::signal_Histogram_IG_Prediction;
				pred.segment_id() = seg_id;
				rc = Send(pred);
			}
		}

		return rc;
	}
	else
		return Send(event);	
}

HRESULT CHistogram_IG_Prediction::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDt = configuration.Read_Double(rsDt_Column, mDt);
	mSampling_Delta = hist_ig_pred::Prediction_Sampling_Delta(mDt);

	memset(&mIG_Course, 0, sizeof(mIG_Course));
	mIst = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IG };
	return S_OK;
}

double CHistogram_IG_Prediction::Update_And_Predict(const double current_time, const double ig_level) {
	double result = std::numeric_limits<double>::quiet_NaN();

	if (SUCCEEDED(mIst->Update_Levels(&current_time, &ig_level, 1))) {
		const size_t hist_index = hist_ig_pred::Level_To_Histogram_Index(ig_level);
		const size_t day_index = static_cast<size_t>(std::floor(current_time)) % 7;

		hist_ig_pred::Inc_Histogram(mIG_Daily, hist_index);
		hist_ig_pred::Inc_Histogram(mIG_Weekly[day_index], hist_index);

		//try to update the patterns based on the 30-minutes long past
		hist_ig_pred::THistogram* history = nullptr;
		hist_ig_pred::TSampling_Delta times = mSampling_Delta + current_time;
		std::array<double, hist_ig_pred::samples_count> levels;
		if (mIst->Get_Continuous_Levels(nullptr, times.data(), levels.data(), levels.size(), scgms::apxNo_Derivation) == S_OK) {
			
			bool all_normals = true;
			for (auto &level : levels)
				if (!std::isnormal(level)) {
					all_normals = false;
						break;
				}

			if (all_normals) {
				history = &mIG_Course[hist_ig_pred::Level_To_Histogram_Index(levels[0])]
									 [hist_ig_pred::Level_To_Histogram_Index(levels[2])]
									 [hist_ig_pred::Level_To_Histogram_Index(levels[2])]
									 [hist_ig_pred::Level_To_Histogram_Index(levels[3])]
									 [hist_ig_pred::Level_To_Histogram_Index(levels[4])];

				hist_ig_pred::Inc_Histogram(*history, hist_index);
			}
		}

		hist_ig_pred::TProbability prob = hist_ig_pred::Histogram_To_Probability(mIG_Daily)*
									      hist_ig_pred::Histogram_To_Probability(mIG_Weekly[day_index]);
		
		if (history != nullptr)
			prob *= hist_ig_pred::Histogram_To_Probability(*history);
		
		
		std::array<size_t, hist_ig_pred::Band_Count> idx;
		std::iota(idx.begin(), idx.end(), 0);
		std::sort(idx.begin(), idx.end(), [&prob](auto a, auto b) {return prob(a) > prob(b);  });

		if (prob[idx[0]] != 0.0) {
			result = hist_ig_pred::Histogram_Index_To_Level(idx[0]);

			const double rel_thresh = 0.5;
			const double rel_diff = abs(result - ig_level) / ig_level;
			if (rel_diff > rel_thresh) {
				if (result > ig_level) result = (1.0 + rel_thresh)*ig_level;
				else result = rel_thresh *ig_level;
			}

		}
			else result = ig_level;
	}

	return result;
}