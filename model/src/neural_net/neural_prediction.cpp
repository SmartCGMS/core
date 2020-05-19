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

#include "neural_prediction.h"

#include "../../../../common/utils/math_utils.h"
#include "../../../../common/utils/DebugHelper.h"

#undef min

CNN_Prediction_Filter::CNN_Prediction_Filter(scgms::IFilter *output)
	: scgms::CBase_Filter(output, neural_prediction::filter_id) {
}

CNN_Prediction_Filter::~CNN_Prediction_Filter() {
	if (mDump_Params)
		Dump_Params();
}

HRESULT CNN_Prediction_Filter::Do_Execute(scgms::UDevice_Event event) {
	bool sent = false;

	auto handle_ig_level = [&event, &sent, this]() {

		const double dev_time = event.device_time();
		const uint64_t seg_id = event.segment_id();
		const double level = event.level();
		const GUID sig_id = event.signal_id();

		HRESULT rc = Send(event);
		sent = SUCCEEDED(rc);
		if (sent) {
			const double predicted_level = Update_And_Predict(seg_id, dev_time, level);

			if (std::isnormal(predicted_level)) {
				scgms::UDevice_Event prediction_event{ scgms::NDevice_Event_Code::Level };
				prediction_event.device_id() = neural_prediction::filter_id;
				prediction_event.level() = predicted_level;
				prediction_event.device_time() = dev_time + mDt;
				prediction_event.signal_id() = neural_net::signal_Neural_Net_Prediction;
				prediction_event.segment_id() = seg_id;
				rc = Send(prediction_event);
			}
		}


		return rc;
	};

	auto free_segment = [&event, this]() {
		auto iter = mIst.find(event.segment_id());
		if (iter != mIst.end())
			mIst.erase(iter);
	};


	HRESULT rc = E_UNEXPECTED;

	switch (event.event_code()) {
	case scgms::NDevice_Event_Code::Level:
		if (event.signal_id() == scgms::signal_IG)
			rc = handle_ig_level();
		break;

	case scgms::NDevice_Event_Code::Time_Segment_Stop:
		free_segment();
		break;

	default: break;
	}

	if (!sent) rc = Send(event);

	return rc;

}


HRESULT CNN_Prediction_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDt = configuration.Read_Double(rsDt_Column, mDt);
	
	return Is_Any_NaN(mDt) ? E_INVALIDARG : S_OK;
}

double CNN_Prediction_Filter::Update_And_Predict(const uint64_t segment_id, const double current_time, const double current_level) {
	auto seg_iter = mIst.find(segment_id);
	if (seg_iter == mIst.end()) {
		scgms::SSignal new_ist = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IG };
		if (new_ist) {
			auto inserted = mIst.insert(std::make_pair(segment_id, new_ist));
			seg_iter = inserted.first;
		}
		else
			return std::numeric_limits<double>::quiet_NaN();	//a failure with this segment
	}

	auto ist = seg_iter->second;

	//1st phase - learning
	Learn(ist, current_time, current_level);

	//2nd phase - obtaining a learned result
	return Predict(ist, current_time);
}

void CNN_Prediction_Filter::Learn(scgms::SSignal& ist, const double current_time, const double current_ig_level) {
	if (SUCCEEDED(ist->Update_Levels(&current_time, &current_ig_level, 1))) {
		auto [pattern, band_index, classified_ok] = Classify(ist, current_time - mDt);

		if (classified_ok)
			mPatterns[static_cast<size_t>(pattern)][band_index].Update(current_ig_level);
	}
}


void CNN_Prediction_Filter::Dump_Params() {
	for (size_t pattern_idx = 0; pattern_idx < static_cast<size_t>(neural_net::NPattern::count); pattern_idx++) {
		for (size_t band_idx = 0; band_idx < neural_net::Band_Count; band_idx++) {

			const auto& pattern = mPatterns[pattern_idx][band_idx];

			dprintf(pattern_idx);
			dprintf("-");
			dprintf(band_idx);
			dprintf("\t\t");

			pattern.Dump_Params();
		}
	}
}

std::tuple<neural_net::NPattern, size_t, bool> CNN_Prediction_Filter::Classify(scgms::SSignal& ist, const double current_time) {
	std::tuple<neural_net::NPattern, size_t, bool> result{ neural_net::NPattern::steady, neural_net::Band_Count / 2, false };

	std::array<double, 3> levels;
	const std::array<double, 3> times = { current_time - 10.0 * scgms::One_Minute,
										  current_time - 5.0 * scgms::One_Minute,
										  current_time };
	if (SUCCEEDED(ist->Get_Continuous_Levels(nullptr, times.data(), levels.data(), levels.size(), scgms::apxNo_Derivation))) {
		if (!Is_Any_NaN(levels)) {
			std::get<NClassify::success>(result) = true;	//classified ok
			std::get<NClassify::band>(result) = const_neural_net::Level_2_Band_Index(levels[2]);

			const double velocity = levels[2] - levels[0];

			if (velocity != 0.0) {
				const double d1 = fabs(levels[1] - levels[0]);
				const double d2 = fabs(levels[2] - levels[1]);
				const bool accelerating = d2 > d1;


				if (velocity > 0.0) {
					std::get<NClassify::pattern>(result) = accelerating && ((levels[1] > levels[0]) && (levels[2] > levels[1])) ?
						neural_net::NPattern::accel : neural_net::NPattern::up;
				}
				else if (velocity < 0.0) {
					std::get<NClassify::pattern>(result) = accelerating && ((levels[1] < levels[0]) && (levels[2] < levels[1])) ?
						neural_net::NPattern::deccel : neural_net::NPattern::down;
				}

			} //else already set to the steady pattern - see result declaration

		}
	}

	return result;
}

double CNN_Prediction_Filter::Predict(scgms::SSignal& ist, const double current_time) {
	double predicted_level = std::numeric_limits<double>::quiet_NaN();
	auto [pattern, pattern_band_index, classified_ok] = Classify(ist, current_time);
	if (classified_ok) {

		const auto& patterns = mPatterns[static_cast<size_t>(pattern)];

		size_t band_counter = 0;
		for (size_t band_idx = 0; band_idx < neural_net::Band_Count; band_idx++) {
			const auto& pattern = patterns[band_idx];

			if (pattern.Valid()) {
				const double dbl_band_idx = static_cast<double>(band_idx);
				mA(band_counter, 0) = dbl_band_idx * dbl_band_idx;
				mA(band_counter, 1) = dbl_band_idx;
				mA(band_counter, 2) = 1.0;
				mb(band_counter) = pattern.Level();

				band_counter++;
			}
		}


		if (band_counter > 3) {	//require an overfit, no >=			
			const Eigen::Vector3d coeff = mA.topRows(band_counter).fullPivHouseholderQr().solve(mb.topRows(band_counter));

			const double dlb_pattern_band_index = static_cast<double>(pattern_band_index);			
			predicted_level = coeff[0] * dlb_pattern_band_index * dlb_pattern_band_index +
				coeff[1] * dlb_pattern_band_index +
				coeff[2];

			if (!patterns[pattern_band_index].Valid()) {
				constexpr double min_level = neural_net::Low_Threshold - neural_net::Half_Band_Size;
				constexpr double max_level = neural_net::High_Threshold + neural_net::Half_Band_Size;

				predicted_level = std::min(predicted_level, max_level);
				predicted_level = std::max(predicted_level, min_level);
			}
		}		
	}

	return predicted_level;
}