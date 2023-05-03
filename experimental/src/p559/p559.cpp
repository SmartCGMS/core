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

#include "p559.h"
#include "../descriptor.h"
#include "../../../../common/utils/math_utils.h"
#include "../../../../common/rtl/SolverLib.h"

#include "../../../../common/utils/DebugHelper.h"

#include <algorithm>

using namespace scgms::literals;

constexpr bool Fully_Self_Driven = false;
constexpr bool Partially_Self_Driven = false;
constexpr size_t Num_Values_To_Self_Drive = 30;
constexpr size_t Num_Values_To_Adjust = 60;

constexpr const double predictionHorizon = 120.0_min;

static_assert(!(Fully_Self_Driven && Partially_Self_Driven), "Cannot have a model that is fully and partially self-driven at the same time");

/* p559 model */

CP559_Model::CP559_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: scgms::CBase_Filter(output), mParameters(scgms::Convert_Parameters<p559_model::TParameters>(parameters, p559_model::default_parameters))
{
	mIs_Self_Driven = Fully_Self_Driven;
	if constexpr (Partially_Self_Driven)
		mSelf_Drive_Countdown = Num_Values_To_Self_Drive;
}

HRESULT CP559_Model::Do_Execute(scgms::UDevice_Event event)
{
	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_Carb_Intake)
		{
			mStored_Carbs.push_back({
				event.device_time(),
				static_cast<double>(event.device_time() + 5.0_hr),
				event.level()
			});
		}
		else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus || event.signal_id() == scgms::signal_Delivered_Insulin_Bolus)
		{
			mStored_Insulin.push_back({
				event.device_time(),
				static_cast<double>(event.device_time() + 5.0_hr),
				event.level() // U -> mU
			});
		}

		if (!mIs_Self_Driven)
		{
			if (event.signal_id() == scgms::signal_IG)
			{
				for (size_t i = 1; i < mPast_IG.size(); i++)
				{
					mPast_IG[i - 1] = mPast_IG[i];
				}
				*mPast_IG.rbegin() = event.level();
			}

			if constexpr (Partially_Self_Driven)
			{
				mSelf_Drive_Countdown--;
				if (mSelf_Drive_Countdown == 0)
				{
					mIs_Self_Driven = true;
					mSelf_Drive_Countdown = Num_Values_To_Adjust;
				}
			}
		}
		else if constexpr (Partially_Self_Driven)
		{
			mSelf_Drive_Countdown--;
			if (mSelf_Drive_Countdown == 0)
			{
				mIs_Self_Driven = false;
				mSelf_Drive_Countdown = Num_Values_To_Self_Drive;
			}
		}
	}

	return mOutput.Send(event);
}

HRESULT CP559_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	// configured in the constructor
	return S_OK;
}

HRESULT IfaceCalling CP559_Model::Initialize(const double current_time, const uint64_t segment_id)
{
	mSegment_Id = segment_id;
	mCurrent_Time = current_time;

	std::fill(mPast_IG.begin(), mPast_IG.end(), mParameters.IG0);

	return S_OK;
}

HRESULT IfaceCalling CP559_Model::Step(const double time_advance_delta)
{
	if (Is_Any_NaN(mCurrent_Time) || time_advance_delta < 0.0)
		return E_ILLEGAL_STATE_CHANGE;

	double est = *mPast_IG.rbegin();

	auto pd = [](const double a, const double b) {
		return a / std::sqrt(1 + b*b);
	};

	auto plog = [](const double x) {
		return std::log(1 + std::abs(x));
	};

	auto psqrt = [](const double x) {
		return std::sqrt(std::abs(x));
	};

	if (time_advance_delta >= 0.0)
	{
		////////// Gamma

		//GE1
		/*const double IGnow = *mPast_IG.rbegin();
		const double IG10 = *(mPast_IG.rbegin() + 2);

		double Gamma = IGnow + std::sin(IGnow - IG10);*/

		//GE3
		/*const double IGnow = *mPast_IG.rbegin();
		const double IG20 = *(mPast_IG.rbegin() + 4);

		double Gamma = IGnow - std::tanh(IG20 - IGnow);*/

		//pre1
		/*const double IGnow = *mPast_IG.rbegin();
		const double IG30 = *(mPast_IG.rbegin() + 6);
		const double IG50 = *(mPast_IG.rbegin() + 10);

		double Gamma = std::sin(psqrt(IG30) + pd(IG50, 75.0));*/

		//pre2
		const double IGnow = *mPast_IG.rbegin();
		const double IG35 = *(mPast_IG.rbegin() + 7);

		double Gamma = psqrt(IG35);

		////////// Theta

		//GE1
		//double Theta = std::sqrt(Calc_IOB_At(mCurrent_Time));

		//GE3
		//double Theta = std::sin(Calc_IOB_At(mCurrent_Time + 10.0_min));

		//pre1
		//double Theta = Calc_IOB_At(mCurrent_Time + 90.0_min);

		//pre2
		double Theta = std::exp(std::sin(Calc_IOB_At(mCurrent_Time + 85.0_min)));

		////////// Omega

		//GE1
		/*const double cob25 = Calc_COB_At(mCurrent_Time + 25.0_min);
		const double cob25_guarded = cob25 == 0 ? 0 : std::log(cob25);

		double Omega = cob25_guarded;*/

		//GE3
		/*const double cob_now = Calc_COB_At(mCurrent_Time);
		const double cob_now_guarded = cob_now == 0 ? 0 : std::log(cob_now);

		const double shift = mParameters.omega_shift;

		const double Cob15 = Calc_COB_At(mCurrent_Time + 15.0_min);
		const double Cob5 = Calc_COB_At(mCurrent_Time + 5.0_min);

		double Omega = std::sin(Cob5 + cob_now_guarded);
		
		//if (Cob15 > 0)
			Omega += Cob15 + shift;*/

		double Omega = pd(std::sin(Calc_COB_At(mCurrent_Time + 80.0_min)), Calc_COB_At(mCurrent_Time + 120.0_min));

		dprintf("Gamma %llf, Theta %llf, Omega %llf\r\n", Gamma, Theta, Omega);

		////////// Estimation

		//GE1
		//est = Gamma + std::sin(std::sqrt(std::max(Theta + Omega, 0.0)));

		//GE3
		//est = Gamma + Theta - Omega;

		//pre1
		//est = IGnow + Gamma * psqrt(Theta * Omega);

		//pre2
		est = IGnow + Gamma - Theta - Omega;

		if (mIs_Self_Driven)
		{
			for (size_t i = 1; i < mPast_IG.size(); i++)
			{
				mPast_IG[i - 1] = mPast_IG[i];
			}

			*mPast_IG.rbegin() = est;
		}

		mCurrent_Time += time_advance_delta;

		auto cleanup = [this](auto& cont) {
			for (auto itr = cont.begin(); itr != cont.end(); )
			{
				if (itr->time_end + predictionHorizon < mCurrent_Time)
					itr = cont.erase(itr);
				else
					++itr;
			}
		};

		cleanup(mStored_Insulin);
		cleanup(mStored_Carbs);
	}

	Emit_Level(mCurrent_Time + predictionHorizon, est, p559_model::ig_signal_id);

	Emit_Level(mCurrent_Time, Calc_IOB_At(mCurrent_Time), scgms::signal_IOB);
	Emit_Level(mCurrent_Time, Calc_COB_At(mCurrent_Time), scgms::signal_COB);

	return S_OK;
}

double CP559_Model::Calc_IOB_At(const double time)
{
	double iob_acc = 0.0;

	constexpr double t_max = 55; //min !!!
	constexpr double Vi = 0.12;
	constexpr double ke = 0.138;

	double S1 = 0, S2 = 0, I = 0;
	double T = 0;

	if (!mStored_Insulin.empty())
	{

		for (auto& r : mStored_Insulin)
		{

			//y=(1-tanh(5(x-0.4)))/2
			/*
			if (time >= r.time_start && time < r.time_end)
			{
				const double time_ratio = (time - r.time_start) / (r.time_end - r.time_start);

				const double iob_base = (((1.0 - std::tanh(5.0 * (time_ratio - 0.4))) * 0.5) * r.value);

				iob_acc += iob_base;
			}
			*/

			/*if (time >= r.time_start && time < r.time_end)
			{
				const double t = (time - r.time_start) / scgms::One_Minute;

				iob_acc += (r.value * t * std::exp(-t / t_max)) / std::pow(t_max, 2.0);
			}*/

			if (T == 0)
			{
				T = r.time_start;
				S1 += r.value;
			}
			else
			{
				while (T < r.time_start && T < time)
				{
					T += scgms::One_Minute;

					I += (S2 / (Vi * t_max)) - (ke * I);
					S2 += (S1 - S2) / t_max;
					S1 += 0 - S1 / t_max;
				}

				S1 += r.value;
			}
		}

		while (T < time)
		{
			T += scgms::One_Minute;

			I += (S2 / (Vi * t_max)) - (ke * I);
			S2 += (S1 - S2) / t_max;
			S1 += 0 - S1 / t_max;
		}

	}

	iob_acc = I;

	return iob_acc;
}

double CP559_Model::Calc_COB_At(const double time)
{
	double cob_acc = 0.0;

	//C(t) = (Dg * Ag * t * e^(-t/t_max)) / t_max^2

	constexpr double t_max = 40; //min
	constexpr double Ag = 0.8;

	constexpr double GlucMolar = 180.16; // glucose molar weight (g/mol)
	constexpr double gCHO_to_mmol = 10 * (1000 / GlucMolar); // 1000 -> g to mg, 180.16 - molar weight of glucose; g -> mmol

	for (auto& r : mStored_Carbs)
	{
		if (time >= r.time_start && time < r.time_end)
		{
			const double t = (time - r.time_start) / scgms::One_Minute; // device_time (rat time) to minutes

			cob_acc += (gCHO_to_mmol*r.value * Ag * t * std::exp(-t/t_max) ) / std::pow(t_max, 2.0);
		}
	}

	return cob_acc;
}

bool CP559_Model::Emit_Level(double time, double value, const GUID& signal_id)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.device_id() = p559_model::model_id;
	evt.signal_id() = signal_id;
	evt.device_time() = time;
	evt.level() = value;
	evt.segment_id() = mSegment_Id;
	
	return Succeeded(mOutput.Send(evt));
}
