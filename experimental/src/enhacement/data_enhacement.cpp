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

#include "data_enhacement.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/utils/math_utils.h"

#include "../../../../common/utils/DebugHelper.h"

using namespace scgms::literals;

CData_Enhacement_Model::CData_Enhacement_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<data_enhacement::TParameters>(parameters, data_enhacement::default_parameters.vector))
{
	//
}

HRESULT CData_Enhacement_Model::Do_Execute(scgms::UDevice_Event event)
{
	if (event.event_code() == scgms::NDevice_Event_Code::Level && event.segment_id() == mSegment_Id)
	{
		if (event.signal_id() == scgms::signal_Carb_Intake)
		{
			mCarbs.push_back({
				event.device_time(),
				event.level()
			});
		}
		else if (event.signal_id() == scgms::signal_Delivered_Insulin_Bolus || event.signal_id() == scgms::signal_Requested_Insulin_Bolus)
		{
			mInsulin.push_back({
				event.device_time(),
				event.level()
			});
		}
		else if (event.signal_id() == scgms::signal_IG)
		{
			mIG.push_back({
				event.device_time(),
				event.level()
			});

			Detect();
		}

		scgms::TDevice_Event* rawptr;
		event->Raw(&rawptr);

		mHeld_Events.push_back(*rawptr);

		return S_OK;
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		Flush();

		return mOutput.Send(event);
	}

	return mOutput.Send(event);
	//return S_OK;
}

void CData_Enhacement_Model::Flush()
{
	std::sort(mHeld_Events.begin(), mHeld_Events.end(), [](const scgms::TDevice_Event& a, const scgms::TDevice_Event& b) {
		return a.device_time < b.device_time;
	});

	for (auto it = mHeld_Events.begin(); it != mHeld_Events.end(); it++)
	{
		const auto& e = *it;

		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

		evt.device_id() = e.device_id;
		evt.device_time() = e.device_time;
		evt.level() = e.level;
		evt.segment_id() = e.segment_id;
		evt.signal_id() = e.signal_id;

		mOutput.Send(evt);
	}

	mHeld_Events.clear();
}

HRESULT CData_Enhacement_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	return S_OK;
}

HRESULT IfaceCalling CData_Enhacement_Model::Initialize(const double current_time, const uint64_t segment_id)
{
	mCurrent_Time = current_time;
	mSegment_Id = segment_id;

	return S_OK;
}

HRESULT IfaceCalling CData_Enhacement_Model::Step(const double time_advance_delta)
{
	if (time_advance_delta < 0)
		return E_ILLEGAL_STATE_CHANGE;

	mCurrent_Time += time_advance_delta;

	return S_OK;
}

void CData_Enhacement_Model::Detect()
{
	double avg = 0.0;
	size_t valueCount = 0;

	double leftmost = std::numeric_limits<double>::quiet_NaN();
	double rightmost = std::numeric_limits<double>::quiet_NaN();

	for (auto itr = mIG.rbegin(); itr != mIG.rend(); itr++)
	{
		const auto& sig = *itr;

		if (sig.time < mCurrent_Time - mParameters.sampling_horizon)
			break;

		avg = (avg * static_cast<double>(valueCount) + sig.value) / static_cast<double>(valueCount + 1);
		valueCount++;

		leftmost = sig.value;
		if (Is_Any_NaN(rightmost))
			rightmost = sig.value;
	}

	if (Is_Any_NaN(leftmost, rightmost, avg) || leftmost == rightmost)
		return;

	const double diff = rightmost - leftmost;

	double carbAccumulator = 0.0;
	double insulinAccumulator = 0.0;

	for (auto itr = mCarbs.rbegin(); itr != mCarbs.rend(); itr++)
	{
		const auto& sig = *itr;

		if (sig.time > mCurrent_Time - mParameters.sampling_horizon)
			continue;

		if (sig.time < mCurrent_Time - mParameters.sampling_horizon - 4 * mParameters.response_delay)
			continue;

		carbAccumulator += sig.value;
	}

	for (auto itr = mInsulin.rbegin(); itr != mInsulin.rend(); itr++)
	{
		const auto& sig = *itr;

		if (sig.time > mCurrent_Time - mParameters.sampling_horizon + mParameters.response_delay)
			continue;

		if (sig.time < mCurrent_Time - mParameters.sampling_horizon - 4 * mParameters.response_delay)
			continue;

		insulinAccumulator += sig.value;
	}

	//constexpr double CarbToInsRatio = 0.2; // 1U for approx 15g of glucose

	/*
	constexpr double PosFactorShift = 1.0;
	constexpr double NegFactorShift = 0.5;

	constexpr double PosFactorToGlucose = 0.2;
	constexpr double NegFactorToGlucose = 3.5; // the higher, the less sensitive
	*/

	const double expectedChangeFactorPos = carbAccumulator;// -insulinAccumulator / CarbToInsRatio;
	const double expectedChangeFactorNeg = /*carbAccumulator * CarbToInsRatio */- insulinAccumulator;

	// detect unexplained increase?
	if (diff > expectedChangeFactorPos * mParameters.pos_factor + mParameters.pos_factor_shift)
	{
		double compensation = (diff - expectedChangeFactorPos * mParameters.pos_factor) / (mParameters.pos_factor);

		mCarbs.push_back({
			mCurrent_Time - mParameters.sampling_horizon - mParameters.response_delay,
			compensation
		});

		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
		evt.device_time() = mCurrent_Time - mParameters.sampling_horizon - mParameters.response_delay;
		evt.device_id() = data_enhacement::model_id;
		evt.signal_id() = data_enhacement::pos_signal_id;
		evt.segment_id() = mSegment_Id;
		evt.level() = compensation;
		//mOutput.Send(evt);

		scgms::TDevice_Event* rawptr;
		evt->Raw(&rawptr);
		mHeld_Events.push_back(*rawptr);
	}

	// detect unexplained decrease?
	if (diff < expectedChangeFactorNeg * mParameters.neg_factor - mParameters.neg_factor_shift)
	{
		double compensation = (expectedChangeFactorNeg * mParameters.neg_factor - diff) / (mParameters.neg_factor);

		mInsulin.push_back({
			mCurrent_Time - mParameters.sampling_horizon - mParameters.response_delay,
			compensation
		});

		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
		evt.device_time() = mCurrent_Time - mParameters.sampling_horizon - mParameters.response_delay;
		evt.device_id() = data_enhacement::model_id;
		evt.signal_id() = data_enhacement::neg_signal_id;
		evt.segment_id() = mSegment_Id;
		evt.level() = compensation;
		//mOutput.Send(evt);

		scgms::TDevice_Event* rawptr;
		evt->Raw(&rawptr);
		mHeld_Events.push_back(*rawptr);
	}
}
