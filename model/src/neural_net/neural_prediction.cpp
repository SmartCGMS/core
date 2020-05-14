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
	bool sent = false;

	auto handle_level = [&event, &sent, this]() {				
		HRESULT rc = E_UNEXPECTED;

		const size_t sig_idx = neural_prediction::CSegment_Signals::Signal_Id_2_Idx(event.signal_id());
		
		if (sig_idx != neural_prediction::CSegment_Signals::invalid) {

			const double dev_time = event.device_time();
			const uint64_t seg_id = event.segment_id();
			const double level = event.level();
			const GUID sig_id = event.signal_id();

			rc = Send(event);
			sent = SUCCEEDED(rc);
			if (sent) {
				mRecent_Predicted_Level = Update_And_Predict(sig_idx, seg_id, dev_time, level);

				if (std::isnormal(mRecent_Predicted_Level)) {
					scgms::UDevice_Event pred{ scgms::NDevice_Event_Code::Level };
					pred.device_id() = neural_prediction::filter_id;
					pred.level() = mRecent_Predicted_Level;
					pred.device_time() = dev_time + mDt;
					pred.signal_id() = neural_net::signal_Neural_Net_Prediction;
					pred.segment_id() = seg_id;
					rc = Send(pred);
				}
			}
		}
		else
			rc = S_OK;
		
		return rc;
	};

	auto free_segment = [&event, this]() {
		auto iter = mSignals.find(event.segment_id());
		if (iter != mSignals.end())
			mSignals.erase(iter);
	};


	HRESULT rc = E_UNEXPECTED;

	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Level:			
			rc = handle_level();
			break;

		case scgms::NDevice_Event_Code::Time_Segment_Start:
			mRecent_Predicted_Level = std::numeric_limits<double>::quiet_NaN();
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
			double delta;
			typename const_neural_net::CNeural_Network::TInput input = Prepare_Input(signals, current_time - mDt, delta);

			if (!std::isnan(input(0))) {			
				mPatterns[input].Update(current_level, delta);

//				for (size_t i=0; i<100; i++)
//					mNeural_Net.Learn(input, const_neural_net::Level_2_Output(current_level));
			}
		}
	}
}

double CNN_Prediction_Filter::Predict_Conservative(neural_prediction::CSegment_Signals& signals, const double current_time) {
	double result = std::numeric_limits<double>::quiet_NaN();

	double delta;
	typename const_neural_net::CNeural_Network::TInput input = Prepare_Input(signals, current_time, delta);
	if (!std::isnan(input(0))) {
		//Do we know this pattern as a reliable one, or should we use NN compute it?
		auto pattern = mPatterns.find(input);
		const bool reliable_pattern = (pattern != mPatterns.end());//&& pattern->second.Is_Reliable();

		if (reliable_pattern) {
			result = pattern->second.Level();
			mKnown_Counter++;
		}
		else {
			result = Predict_Poly(signals, current_time);
//			if (mKnown_Counter>100)
//				result = const_neural_net::Output_2_Level(mNeural_Net.Forward(input));		
			mUnknown_Counter++;
		}
	}

	return result;
}

typename const_neural_net::CNeural_Network::TInput CNN_Prediction_Filter::Prepare_Input(neural_prediction::CSegment_Signals& signals, const double current_time, double& delta) {
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
		
		input(2) = const_neural_net::Level_2_Input(ist[2]);		

		input(3) = 0;// iob[2] - iob[0] > 0.0 ? 1.0 : -1.0;
		input(4) = 0;// cob[2] - cob[0] > 0.0 ? 1.0 : -1.0;

		if (cob[2] != 0.0) {
			//input(0) = 1.0;
			//input(1) = 1.0;
			//input(1) = cob[2] > cob[0] ? 1.0 : -1.0;
		}

		delta = ist[2] - ist[1];
	}
	else
		delta = std::numeric_limits<double>::quiet_NaN();

	return input;
}


void CNN_Prediction_Filter::Dump_Params() {
	dprintf("Known patterns: ");
	dprintf(mKnown_Counter);
	dprintf("\nUnknown patterns: ");
	dprintf(mUnknown_Counter);
	dprintf("\n");
}



double CNN_Prediction_Filter::Predict_Poly(neural_prediction::CSegment_Signals& signals, const double current_time) {
	double result = std::numeric_limits<double>::quiet_NaN();

	double delta;
	typename const_neural_net::CNeural_Network::TInput input = Prepare_Input(signals, current_time, delta);
	if (!std::isnan(input(0))) {	
		if (input(1) == 0.0) return mRecent_Predicted_Level;

		std::vector<double> x, y;
		for (const auto & pattern: mPatterns) {
			if ((pattern.first(0) == input(0)) && (pattern.first(1) == input(1))) {
				x.push_back(pattern.first(2));
				y.push_back(pattern.second.Level());
			}
		}

		if (x.size() > 3) {
			Eigen::Matrix<double, Eigen::Dynamic, 3> A;
			Eigen::Matrix<double, Eigen::Dynamic, 1> b;

			A.resize(x.size(), Eigen::NoChange);
			b.resize(x.size(), Eigen::NoChange);

			for (size_t i = 0; i < x.size(); i++) {
				A(i, 0) = x[i] * x[i]; A(i, 1) = x[i]; A(i, 2) = 1; b(i) = y[i];
			}

			const Eigen::Vector3d coeff = A.fullPivHouseholderQr().solve(b);

			result = coeff[0] * input(2)*  input(2) +
				coeff[1] * input(2) +
				coeff[2];			


			auto pattern = mPatterns.find(input);
			if (pattern != mPatterns.end()) {
				pattern->second.Apply_Max_Delta(result, mRecent_Predicted_Level);
			} else {
				result = std::min(result, const_neural_net::Band_Index_2_Level(neural_net::Band_Count - 1));
				result = std::max(result, const_neural_net::Band_Index_2_Level(0));
			}
		}

	}

	return result;
}

double CNN_Prediction_Filter::Predict_Akima(neural_prediction::CSegment_Signals& signals, const double current_time) {
	double result = std::numeric_limits<double>::quiet_NaN();

	double delta;
	typename const_neural_net::CNeural_Network::TInput input = Prepare_Input(signals, current_time, delta);
	if (!std::isnan(input(0))) {
		std::vector<double> x, y;
		for (const auto& pattern : mPatterns) {
			if ((pattern.first(0) == input(0)) && (pattern.first(1) == input(1))) {
				x.push_back(pattern.first(2));
				y.push_back(pattern.second.Level());
			}
		}

		if (x.size() > 3) {
			scgms::SSignal approx{ scgms::STime_Segment{}, scgms::signal_BG };

			approx->Update_Levels(x.data(), y.data(), x.size());

			approx->Get_Continuous_Levels(nullptr, &input(2), &result, 1, scgms::apxNo_Derivation);
		}

	}

	return result;
}

double CNN_Prediction_Filter::Predict(neural_prediction::CSegment_Signals& signals, const double current_time) {
	double result = Predict_Poly(signals, current_time);

	return result;
}

