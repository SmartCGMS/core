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

#include "bases.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/ApproxLib.h"
#include "../../../../common/utils/math_utils.h"

#include "../../../../common/utils/DebugHelper.h"

//#include "../pattern_prediction/pattern_prediction_descriptor.h"

enum class NIG_History_Mode
{
	Moving_Average,
	Median,
	Moving_Average_Plus_StdDev,
};

constexpr NIG_History_Mode IG_History_Mode = NIG_History_Mode::Moving_Average;

using namespace scgms::literals;

CBase_Functions_Predictor::CBase_Functions_Predictor(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<bases_pred::TParameters>(parameters, bases_pred::default_parameters.vector)) {
	//

	mTime_Seg = refcnt::Manufacture_Object_Shared<scgms::CTime_Segment, scgms::ITime_Segment, scgms::STime_Segment>();
}

HRESULT CBase_Functions_Predictor::Do_Execute(scgms::UDevice_Event event)
{
	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_IG)
		{
			mStored_IG.push_back({
				event.device_time(), event.device_time(), event.level()
			});

			for (auto itr = mStored_IG.begin(); itr != mStored_IG.end(); )
			{
				if (itr->time_start < mCurrent_Time - mParameters.baseAvgTimeWindow)
					itr = mStored_IG.erase(itr);
				else // once we find a value that falls within the window, end the cleanup - values are always ordered from oldest to newest
					break;
			}

			double times = event.device_time();
			double levels = event.level();
			auto ig = mTime_Seg.Get_Signal(scgms::signal_IG);
			ig->Update_Levels(&times, &levels, 1);
		}
		else if (event.signal_id() == scgms::signal_Carb_Intake)
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
				event.level()
			});
		}
		else if (event.signal_id() == scgms::signal_Physical_Activity)
		{
			mLast_Physical_Activity_Index = event.level();
		}
		/*else if (event.signal_id() == pattern_prediction::signal_Pattern_Prediction)
		{
			mLast_Pattern_Pred = event.level();
		}*/
	}

	return mOutput.Send(event);
}

HRESULT CBase_Functions_Predictor::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	return S_OK;
}

HRESULT IfaceCalling CBase_Functions_Predictor::Initialize(const double current_time, const uint64_t segment_id)
{
	mCurrent_Time = current_time;
	mLast_Activated_Day = std::floor(mCurrent_Time);
	mSegment_Id = segment_id;

	return S_OK;
}

HRESULT IfaceCalling CBase_Functions_Predictor::Step(const double time_advance_delta)
{
	if (time_advance_delta > 0)
	{
		mCurrent_Time += time_advance_delta;

		if (mCurrent_Time + 3.0 > mLast_Activated_Day)
		{
			for (size_t i = 0; i < bases_pred::Base_Functions_Count; i++)
			{
				mActive_Bases.push_back({
					i,
					mLast_Activated_Day + mParameters.baseFunction[i].tod_offset
				});
			}

			mLast_Activated_Day += 1.0;
		}

		if (mStored_IG.size() > 2)
		{
			size_t valCnt = 0;
			double val = 0.0;

			if constexpr (IG_History_Mode == NIG_History_Mode::Moving_Average || IG_History_Mode == NIG_History_Mode::Moving_Average_Plus_StdDev)
			{
				for (auto itr = mStored_IG.rbegin(); itr != mStored_IG.rend(); ++itr)
				{
					if (itr->time_start < mCurrent_Time - mParameters.baseAvgTimeWindow)
						break;

					valCnt++;
					val += itr->value;
				}

				if (valCnt > 1)
					val /= static_cast<double>(valCnt);

				if constexpr (IG_History_Mode == NIG_History_Mode::Moving_Average_Plus_StdDev)
				{
					double sd = 0;

					for (auto itr = mStored_IG.rbegin(); itr != mStored_IG.rend(); ++itr)
					{
						if (itr->time_start < mCurrent_Time - mParameters.baseAvgTimeWindow)
							break;

						sd += std::pow(itr->value - val, 2.0);
					}

					if (valCnt > 1)
					{
						sd /= static_cast<double>(valCnt + 1); // bessel's correction
						sd = std::sqrt(sd);
					}

					val += sd;
				}
			}
			else if constexpr (IG_History_Mode == NIG_History_Mode::Median)
			{
				std::vector<double> d;

				for (auto itr = mStored_IG.rbegin(); itr != mStored_IG.rend(); ++itr)
				{
					if (itr->time_start < mCurrent_Time - mParameters.baseAvgTimeWindow)
						break;

					d.push_back(itr->value);
				}
				// i am so sorry
				std::sort(d.begin(), d.end());
				if (d.size() > 1)
				{
					if (d.size() % 2)
						val = (d[d.size() / 2 - 1] + d[d.size() / 2]) / 2.0;
					else
						val = d[d.size() / 2];
				}
				else
					val = d.empty() ? 0 : d[0];
			}

			const double ppredContribFactor = (0.0 + mParameters.paContrib * mLast_Pattern_Pred);

			double est = (1 - mParameters.curWeight) * mStored_IG.rbegin()->value + mParameters.curWeight * (val + mParameters.baseAvgOffset) * ppredContribFactor;

			// emit just the weighted average and current value
			/*{
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = scgms::signal_Virtual[55];
				evt.device_id() = bases_pred::model_id;
				evt.device_time() = mCurrent_Time + Prediction_Horizon;
				evt.segment_id() = mSegment_Id;
				evt.level() = est;
				mOutput.Send(evt);
			}*/

			if (Is_Any_NaN(mLast_Physical_Activity_Index))
				mLast_Physical_Activity_Index = 0;

			//if (Is_Any_NaN(mLast_Pattern_Pred))
				//mLast_Pattern_Pred = 0;

			const double choContribFactor = (1.0 + mParameters.carbContrib * Calc_COB_At(mCurrent_Time + mParameters.carbPast));
			const double insContribFactor = (1.0 + mParameters.insContrib  * Calc_IOB_At(mCurrent_Time + mParameters.insPast) );
			const double paContribFactor =  (0.0 + mParameters.paContrib * mLast_Physical_Activity_Index);

			double basisFunctionContrib = 0;
			double unfactoredBasisFunctionContrib = 0;

			for (auto itr = mActive_Bases.begin(); itr != mActive_Bases.end(); )
			{
				const auto& fnc = *itr;

				double baseContribFactor = choContribFactor;
				
				if (fnc.idx >= bases_pred::Base_Functions_CHO + bases_pred::Base_Functions_Ins + bases_pred::Base_Functions_PA)
					baseContribFactor = ppredContribFactor;
				else if (fnc.idx >= bases_pred::Base_Functions_CHO + bases_pred::Base_Functions_Ins)
					baseContribFactor = paContribFactor;
				else if (fnc.idx >= bases_pred::Base_Functions_CHO)
					baseContribFactor = insContribFactor;
				else if (fnc.idx >= bases_pred::Base_Functions_Count)
					break;

				const auto& pars = mParameters.baseFunction[fnc.idx];
				double tval = pars.amplitude * std::exp(-std::pow(mCurrent_Time + bases_pred::Prediction_Horizon - fnc.toff, 2.0) / (2 * pars.variance * pars.variance));

				unfactoredBasisFunctionContrib += tval;

				if (fnc.toff + 24.0_hr < mCurrent_Time)
					itr = mActive_Bases.erase(itr);
				else
					++itr;

				basisFunctionContrib += tval * baseContribFactor;
			}

			// emit the basis function contribution
			/*
			{
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = scgms::signal_Steps;
				evt.device_id() = bases_pred::model_id;
				evt.device_time() = mCurrent_Time + Prediction_Horizon;
				evt.segment_id() = mSegment_Id;
				evt.level() = unfactoredBasisFunctionContrib;
				mOutput.Send(evt);
			}
			*/

			est += basisFunctionContrib;

			est += mParameters.c;

			/*double times = mCurrent_Time - mParameters.h;
			double pastlev = 0;
			auto ig = mTime_Seg.Get_Signal(scgms::signal_IG);
			ig->Get_Continuous_Levels(nullptr, &times, &pastlev, 1, scgms::apxNo_Derivation);*/
			const double phiT = bases_pred::Prediction_Horizon;// +mParameters.k * mStored_IG.rbegin()->value * (mStored_IG.rbegin()->value - pastlev) / mParameters.h;

			// emit the prediction
			if (!Is_Any_NaN(phiT))
			{
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = bases_pred::ig_signal_id;
				evt.device_id() = bases_pred::model_id;
				evt.device_time() = mCurrent_Time + phiT;//bases_pred::Prediction_Horizon;
				evt.segment_id() = mSegment_Id;
				evt.level() = est;
				mOutput.Send(evt);
			}
		}

		auto cleanup = [this](auto& cont) {
			for (auto itr = cont.begin(); itr != cont.end(); )
			{
				if (itr->time_end + 120_min < mCurrent_Time)
					itr = cont.erase(itr);
				else
					++itr;
			}
		};

		cleanup(mStored_Insulin);
		cleanup(mStored_Carbs);
	}

	return S_OK;
}

double CBase_Functions_Predictor::Calc_IOB_At(const double time)
{
	double iob_acc = 0.0;

	constexpr double t_max = 55;
	constexpr double Vi = 0.12;
	constexpr double ke = 0.138;

	double S1 = 0, S2 = 0, I = 0;
	double T = 0;

	if (!mStored_Insulin.empty())
	{

		for (auto& r : mStored_Insulin)
		{
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

double CBase_Functions_Predictor::Calc_COB_At(const double time)
{
	double cob_acc = 0.0;

	//C(t) = (Dg * Ag * t * e^(-t/t_max)) / t_max^2

	constexpr double t_max = 40; //min
	constexpr double Ag = 0.8;

	constexpr double GlucMolar = 180.16; // glucose molar weight (g/mol)
	constexpr double gCHO_to_mmol = (1000 / GlucMolar); // 1000 -> g to mg, 180.16 - molar weight of glucose; g -> mmol
	constexpr double mmol_to_gCHO = 1.0 / gCHO_to_mmol;

	for (auto& r : mStored_Carbs)
	{
		if (time >= r.time_start && time < r.time_end)
		{
			const double t = (time - r.time_start) / scgms::One_Minute; // device_time (rat time) to minutes

			const double Dg = gCHO_to_mmol * r.value;

			cob_acc += (Dg * Ag * t * std::exp(-t / t_max)) / std::pow(t_max, 2.0);
			//cob_acc += Dg * Ag - (Dg * Ag * (t_max - std::exp(-t / t_max) * (t_max + t)) / t_max);
		}
	}

	return cob_acc * mmol_to_gCHO;
}
