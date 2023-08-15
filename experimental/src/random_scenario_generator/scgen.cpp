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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "scgen.h"
#include "../descriptor.h"

#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/utils/math_utils.h"
#include "../../../../common/rtl/rattime.h"

#include <random>

CRandom_Scenario_Generator::CRandom_Scenario_Generator(scgms::IFilter* output) : CBase_Filter(output) {
	//
}

CRandom_Scenario_Generator::~CRandom_Scenario_Generator() {
	if (mGen_Thread && mGen_Thread->joinable())
		mGen_Thread->join();
}

HRESULT IfaceCalling CRandom_Scenario_Generator::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	mMax_Time = configuration.Read_Double(scgen::random::rsMax_Time);
	mBolus_Mean = configuration.Read_Double(scgen::random::rsBolus_Mean, mBolus_Mean);
	mBolus_Range = configuration.Read_Double(scgen::random::rsBolus_Range, mBolus_Range);
	mCarb_Base_Mean = configuration.Read_Double(scgen::random::rsCarb_Base_Mean, mCarb_Base_Mean);
	mCarb_Base_Range = configuration.Read_Double(scgen::random::rsCarb_Base_Range, mCarb_Base_Range);
	mMin_Meals = configuration.Read_Int(scgen::random::rsMeal_Count_Min, mMin_Meals);
	mMax_Meals = configuration.Read_Int(scgen::random::rsMeal_Count_Max, mMax_Meals);
	mForgot_Bolus_Prob = configuration.Read_Double(scgen::random::rsForgotten_Bolus_Probability, mForgot_Bolus_Prob);
	mMeal_Time_Range = configuration.Read_Double(scgen::random::rsMeal_Time_Range, mMeal_Time_Range);

	if (Is_Any_NaN(mMax_Time)) {
		error_description.push(L"Maximum time cannot be NaN");
		return E_FAIL;
	}

	if (mForgot_Bolus_Prob < 0 || mForgot_Bolus_Prob > 1) {
		error_description.push(L"Forgotten bolus probability should be a number between 0 and 1");
		return E_FAIL;
	}

	mGen_Thread = std::make_unique<std::thread>(&CRandom_Scenario_Generator::Generate_Scenario_Thread_Func, this);

	return S_OK;
}

void CRandom_Scenario_Generator::Generate_Scenario_Thread_Func() {

	double now = Unix_Time_To_Rat_Time(time(nullptr));
	const double max = now + mMax_Time;

	double nextUpdate = std::ceil(now);

	struct TScheduled_Meal
	{
		double amount;
		double time;
		double bolusAmount;
	};

	std::random_device rdev;
	std::default_random_engine reng(rdev());
	std::uniform_int_distribution<int64_t> mealCntDist(mMin_Meals, mMax_Meals + 1);
	std::normal_distribution<double> mealAmountDist(mCarb_Base_Mean, mCarb_Base_Range);
	std::normal_distribution<double> mealTimeVarDist(0, scgms::One_Minute * 10);
	std::normal_distribution<double> bolusAmountDist(mBolus_Mean, mBolus_Range);
	std::uniform_real_distribution<double> forgotBolusProb(0, 1);
	std::list<TScheduled_Meal> scheduled;

	Emit_Segment_Marker(now, true);

	while (now < max)
	{
		if (now >= nextUpdate)
		{
			// meals from 6 to 20
			// 20+6 / 2 = 13
			// 13
			const double middlePoint = scgms::One_Hour * 13.0;
			const double rangeVal = scgms::One_Hour * 7.0;

			const auto mealCnt = mealCntDist(reng);
			const double mealSpacing = (rangeVal * 2) / static_cast<double>(mealCnt);
			double curMealTime = middlePoint - rangeVal;
			for (int64_t i = 0; i < mealCnt; i++) {

				const double amount = mealAmountDist(reng);
				const double actualTime = now + curMealTime + mMeal_Time_Range * mealTimeVarDist(reng);
				const double bolusBase = bolusAmountDist(reng) * amount;

				scheduled.push_back(TScheduled_Meal(
					amount,
					actualTime,
					(forgotBolusProb(reng) < mForgot_Bolus_Prob) ? 0 : bolusBase
				));

				curMealTime += mealSpacing;
			}

			nextUpdate += 1.0; // next day
		}

		if (!scheduled.empty()) {
			const auto& ml = *scheduled.begin();
			if (ml.time <= now) {
				Emit_Signal(ml.time, ml.amount, scgms::signal_Carb_Intake);
				if (ml.bolusAmount > 0) {
					Emit_Signal(ml.time, ml.bolusAmount, scgms::signal_Requested_Insulin_Bolus);
				}

				scheduled.erase(scheduled.begin());
			}
		}

		Emit_Signal(now, 0, scgms::signal_Synchronization);
		now += scgms::One_Minute * 5;
	}

	Emit_Segment_Marker(now, false);
}

HRESULT IfaceCalling CRandom_Scenario_Generator::Do_Execute(scgms::UDevice_Event event) {

	if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down) {
		if (mGen_Thread && mGen_Thread->joinable())
			mGen_Thread->join();
	}

	return mOutput.Send(event);
}

void CRandom_Scenario_Generator::Emit_Signal(double time, double value, const GUID& signal_id) {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.device_id() = scgen::random::id;
	evt.device_time() = time;
	evt.level() = value;
	evt.segment_id() = 1;
	evt.signal_id() = signal_id;
	mOutput.Send(evt);
}

void CRandom_Scenario_Generator::Emit_Segment_Marker(double time, bool start) {
	scgms::UDevice_Event evt{ start ? scgms::NDevice_Event_Code::Time_Segment_Start : scgms::NDevice_Event_Code::Time_Segment_Stop };
	evt.device_id() = scgen::random::id;
	evt.device_time() = time;
	evt.segment_id() = 1;
	mOutput.Send(evt);
}
