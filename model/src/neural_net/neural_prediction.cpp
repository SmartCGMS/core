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

#undef min

neural_prediction::CSegment_Signals::CSegment_Signals() {
	signals[ist_idx] = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IG };	
	signals[cob_idx] = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_COB };
	signals[iob_idx] = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IOB };

	if (!refcnt::Shared_Valid_All(signals[0], signals[1], signals[2])) throw std::runtime_error{"Cannot get all the signals!"};
}

bool neural_prediction::CSegment_Signals::Update(const size_t sig_idx, const double time, const double level) {
	return SUCCEEDED(signals[sig_idx]->Update_Levels(&time, &level, 1));
}

bool neural_prediction::CSegment_Signals::Get_Levels(const neural_prediction::TLevels times, 
	                                                 neural_prediction::TLevels& ist, neural_prediction::TLevels& iob, neural_prediction::TLevels& cob) {
	bool data_ok = signals[iob_idx]->Get_Continuous_Levels(nullptr, times.data(), iob.data(), iob.size(), scgms::apxNo_Derivation) == S_OK;
	data_ok &= signals[cob_idx]->Get_Continuous_Levels(nullptr, times.data(), cob.data(), cob.size(), scgms::apxNo_Derivation) == S_OK;
	data_ok &= signals[ist_idx]->Get_Continuous_Levels(nullptr, times.data(), ist.data(), ist.size(), scgms::apxNo_Derivation) == S_OK;
	data_ok &= !Is_NaN(iob, cob, ist);

	return data_ok;
}

size_t neural_prediction::CSegment_Signals::Signal_Id_2_Idx(const GUID& id) {
	if (id == scgms::signal_IG) return ist_idx;
	if (id == scgms::signal_COB) return cob_idx;
	if (id == scgms::signal_IOB) return iob_idx;
	return invalid;
}


CNN_Prediction_Filter::CNN_Prediction_Filter(scgms::IFilter *output)
	: scgms::CBase_Filter(output, neural_prediction::filter_id) {
}

CNN_Prediction_Filter::~CNN_Prediction_Filter() {
	if (mDump_Params)
		Dump_Params();
}

HRESULT CNN_Prediction_Filter::Do_Execute(scgms::UDevice_Event event) {
	HRESULT rc = E_UNEXPECTED;
	bool our_signal = event.event_code() == scgms::NDevice_Event_Code::Level;

	if (our_signal) {
		const size_t sig_idx = neural_prediction::CSegment_Signals::Signal_Id_2_Idx(event.signal_id());
		our_signal = sig_idx != neural_prediction::CSegment_Signals::invalid;

		if (our_signal) {

			const double dev_time = event.device_time();
			const uint64_t seg_id = event.segment_id();
			const double level = event.level();
			const GUID sig_id = event.signal_id();

			rc = Send(event);
			if (SUCCEEDED(rc)) {
				const double pred_level = Update_And_Predict(sig_idx, seg_id, dev_time, level);

				if (std::isnormal(pred_level)) {
					scgms::UDevice_Event pred{ scgms::NDevice_Event_Code::Level };
					pred.device_id() = neural_prediction::filter_id;
					pred.level() = pred_level;
					pred.device_time() = dev_time + mDt;
					pred.signal_id() = neural_net::signal_Neural_Net_Prediction;
					pred.segment_id() = seg_id;
					rc = Send(pred);
				}
			}
		}
	}

	if (!our_signal)
		rc = Send(event);

	return rc;
}

HRESULT CNN_Prediction_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDt = configuration.Read_Double(rsDt_Column, mDt);
	
	return Is_NaN(mDt) ? E_INVALIDARG : S_OK;
}

double CNN_Prediction_Filter::Update_And_Predict(const size_t sig_idx, const uint64_t segment_id, const double current_time, const double current_level) {
	double result = std::numeric_limits<double>::quiet_NaN();
 
	auto signals_iter = mSignals.find(segment_id);
	if (signals_iter == mSignals.end()) {
		const auto inserted = mSignals.insert(std::make_pair(segment_id, neural_prediction::CSegment_Signals{}));
		if (inserted.second) return result;	//failure
		signals_iter = inserted.first;
	}

	//1st phase - learning
	Learn(signals_iter->second, sig_idx, current_time, current_level);
		
	
	//2nd phase - obtaining a learned result
	return Predict(signals_iter->second, current_time);	
}

void CNN_Prediction_Filter::Learn(neural_prediction::CSegment_Signals& signals, const size_t sig_idx, const double current_time, const double current_level) {
	if (signals.Update(sig_idx, current_time, current_level)) {

		if (sig_idx == neural_prediction::CSegment_Signals::ist_idx) {
			typename const_neural_net::CNeural_Network::TInput input = Prepare_Input(signals, current_time - mDt);
			if (!std::isnan(input(0))) {
				typename const_neural_net::CNeural_Network::TFinal_Output target;
				double target_level = std::floor(std::min(current_level * 2.0, pow(2.0, target.size())));
				target = static_cast<size_t>(target_level);
				mNeural_Net.Learn(input, target);
			}
		}
	}
}

double CNN_Prediction_Filter::Predict(neural_prediction::CSegment_Signals& signals, const double current_time) {
	double result = std::numeric_limits<double>::quiet_NaN();

	typename const_neural_net::CNeural_Network::TInput input = Prepare_Input(signals, current_time);
	if (!std::isnan(input(0))) 
		result = static_cast<double>(mNeural_Net.Forward(input).to_ulong() * 0.5);	

	return result;
}

typename const_neural_net::CNeural_Network::TInput CNN_Prediction_Filter::Prepare_Input(neural_prediction::CSegment_Signals& signals, const double current_time) {
	typename const_neural_net::CNeural_Network::TInput input;
	input(0) = std::numeric_limits<double>::quiet_NaN();

	std::array<double, 3> ist, iob, cob;	
	std::array<double, 3> times = { current_time - 10.0 * scgms::One_Minute,
									current_time - 5.0 * scgms::One_Minute,
									current_time };
	if (signals.Get_Levels(times, ist, iob, cob)) {
		const double x_raw = ist[2] - ist[0];
		double x2_raw = -1.0;

		if (x_raw != 0.0) {
			const double d1 = fabs(ist[1] - ist[0]);
			const double d2 = fabs(ist[2] - ist[1]);

			//if ((d1 != 0.0) && (d2 > d1)) x2_raw = 1.0;	-- the ifs below are more accurate

			const bool acc = d2 > d1;
			if (acc) {
				if (x_raw > 0.0) {
					if ((ist[1] > ist[0]) && (ist[2] > ist[1])) x2_raw = 1.0;
				}

				if (x_raw < 0.0) {
					if ((ist[1] < ist[0]) && (ist[2] < ist[1])) x2_raw = 1.0;
				}
			}
		}

		if (x_raw != 0.0) input(0) = x_raw > 0.0 ? 1.0 : -1.0;
			else input(0) = 0.0;

		input(1) = x2_raw;

		//input(2) = neural_net::Level_To_Histogram_Index(ist[2]) - static_cast<double>(neural_net::Band_Count) * 0.5;
		//input(2) /= static_cast<double>(neural_net::Band_Count) * 0.5;
		input(2) /= (std::min(16.0, ist[2]) - 8.0) / 8.0;

		input(3) = iob[2] - iob[0] > 0.0 ? 1.0 : -1.0;
		input(4) = cob[2] - cob[0] > 0.0 ? 1.0 : -1.0;
	}

	return input;
}


void CNN_Prediction_Filter::Dump_Params() {
	
}

