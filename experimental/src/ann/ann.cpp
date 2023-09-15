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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "ann.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/ApproxLib.h"
#include "../../../../common/utils/math_utils.h"

#include "../../../../common/utils/DebugHelper.h"

#if !defined(__cplusplus) || __cplusplus < 202002L
template<typename TItr>
void shift_right_single(TItr begin, const TItr end) {
	for (TItr itr = begin; itr != end && (itr + 1) != end; itr++) {
		std::swap(*begin, *(itr + 1));
	}
}
#else
template<typename TItr>
void shift_right_single(TItr begin, const TItr end) {
	std::shift_right(begin, end, 1);
}
#endif

CANN_Model::CANN_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<ann::TParameters>(parameters, ann::default_parameters.data())) {

	mActivations[0].resize(ann::inputs_size + 1); // 6x IG, 2x IoB, 2x CoB, 2x PA + 1x bias
	mActivations[1].resize(ann::hidden_1_size + 1); // + 1x bias
	mActivations[2].resize(ann::hidden_2_size + 1); // + 1x bias
	mActivations[3].resize(2); // "fake" bias

	std::fill(mActivations[0].begin(), mActivations[0].end(), 0);

	mActivations[0][ann::inputs_size] = 1.0; // bias
	mActivations[1][ann::hidden_1_size] = 1.0; // bias
	mActivations[2][ann::hidden_2_size] = 1.0; // bias

	mWeights[0] = mParameters.w_input_to_1;
	mWeights[1] = mParameters.w_1_to_2;
	mWeights[2] = mParameters.w_2_to_output;
}

HRESULT CANN_Model::Do_Execute(scgms::UDevice_Event event)
{
	auto shift_input_in = [this](size_t begin, size_t end, double val) {
		shift_right_single(mActivations[0].begin() + begin, mActivations[0].begin() + end);
		mActivations[0][begin] = val;
	};

	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_IG)
		{
			// 0 = most recent
			if (mActivations[0][ann::ig_history - 1] < 1) {
				std::fill_n(mActivations[0].begin(), ann::ig_history, event.level());
			}
			else {
				shift_input_in(0, ann::ig_history, event.level());

				double acc = 0;
				for (size_t i = 0; i < ann::ig_history - 1; i++) {
					acc += (mActivations[0][i + 1] - mActivations[0][i]) / (scgms::One_Minute*5);
				}
				acc /= static_cast<double>(ann::ig_history - 1);

				shift_input_in(ann::ig_history, ann::ig_history + ann::ig_trend_history, acc);
			}
		}
		else if (event.signal_id() == scgms::signal_COB)
		{
			shift_input_in(ann::ig_history + ann::ig_trend_history, ann::ig_history + ann::ig_trend_history + ann::iob_history, event.level());
		}
		else if (event.signal_id() == scgms::signal_IOB)
		{
			shift_input_in(ann::ig_history + ann::ig_trend_history + ann::iob_history, ann::ig_history + ann::ig_trend_history + ann::iob_history + ann::cob_history, event.level());
		}
		else if (event.signal_id() == scgms::signal_Physical_Activity)
		{
			shift_input_in(ann::ig_history + ann::ig_trend_history + ann::iob_history + ann::cob_history, ann::ig_history + ann::ig_trend_history + ann::iob_history + ann::cob_history + ann::pa_history, event.level());
		}

		/*auto [minel, maxel] = std::minmax_element(mActivations[0].begin(), mActivations[0].end());

		if (*minel != 0 && *maxel != 0 && (*maxel - *minel) != 0) {
			const double factor = 1 / (*maxel - *minel);
			std::transform(mActivations[0].begin(), mActivations[0].end(), mActivations[0].begin(), [factor](double c) { return c * factor; });

			mActivations[0][ann::inputs_size] = 1.0; // bias
			mActivations[1][ann::hidden_1_size] = 1.0; // bias
			mActivations[2][ann::hidden_2_size] = 1.0; // bias
		}*/
	}

	return mOutput.Send(event);
}

HRESULT CANN_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	return S_OK;
}

HRESULT IfaceCalling CANN_Model::Initialize(const double current_time, const uint64_t segment_id)
{
	mCurrent_Time = current_time;
	mSegment_Id = segment_id;

	return S_OK;
}

inline double activation_logistic(const double x)
{
	return 1.0 / (1.0 + std::exp(-x));
}

inline double activation_tanh(const double x)
{
	return std::tanh(x);
}

inline double activation_elu(const double x)
{
	return x >= 0 ? x : (0.9 * std::exp(x) - 1);
}

inline double activation(const double x)
{
	//return activation_elu(x);
	//return activation_logistic(x);
	return activation_tanh(x);
}

void CANN_Model::Feed_Forward()
{
	// 1 - 4 (do not activate input layer, it is activated by inputs)
	/*for (size_t i = 1; i < 4; i++) {
		Feed_Forward_Layer(i);
	}*/

	// written out like this to allow different types of layers
	Feed_Forward_Layer_Classic(1);
	Feed_Forward_Layer_Classic(2);
	Feed_Forward_Layer_Classic(3);
}

void CANN_Model::Feed_Forward_Layer_Classic(size_t layer)
{
	std::fill_n(mActivations[layer].begin(), mActivations[layer].size() - 1, 0);

	for (size_t n = 0; n < mActivations[layer].size() - 1; n++) {
		double actValue = 0;
		for (size_t i = 0; i < mActivations[layer - 1].size(); i++) {
			actValue += mWeights[layer - 1][i] * mActivations[layer - 1][i];
		}

		mActivations[layer][n] = activation(actValue);
	}
}

HRESULT IfaceCalling CANN_Model::Step(const double time_advance_delta)
{
	if (time_advance_delta > 0)
	{
		mCurrent_Time += time_advance_delta;

		Feed_Forward();

		// emit the prediction
		{
			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
			evt.signal_id() = ann::signal_id;
			evt.device_id() = ann::model_id;
			evt.device_time() = mCurrent_Time + scgms::One_Minute * 30;
			evt.segment_id() = mSegment_Id;
			evt.level() = mActivations[3][0] * 25.0; // artificially scale the output
			mOutput.Send(evt);
		}
	}

	return S_OK;
}
