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

#include "bases.h"
#include "../../../../common/rtl/SolverLib.h"

#include "../../../../common/utils/DebugHelper.h"

using namespace scgms::literals;

CBase_Functions_Predictor::CBase_Functions_Predictor(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<bases_pred::TParameters>(parameters, bases_pred::default_parameters.vector)) {
	//
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
			if (event.level() > mLast_Physical_Activity_Index)
				mLast_Physical_Activity_Index = mLast_Physical_Activity_Index * 0.1 + event.level() * 0.9;
			else
				mLast_Physical_Activity_Index = mLast_Physical_Activity_Index * 0.9 + event.level() * 0.1;
			mLast_Physical_Activity_Time = event.device_time();
		}
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
	constexpr double Prediction_Horizon = 60.0_min;

	if (time_advance_delta > 0)
	{
		mCurrent_Time += time_advance_delta;

		if (mCurrent_Time + 2.0 > mLast_Activated_Day)
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

			for (auto itr = mStored_IG.rbegin(); itr != mStored_IG.rend(); ++itr)
			{
				if (itr->time_start < mCurrent_Time - mParameters.baseAvgTimeWindow)
					break;

				valCnt++;
				val += itr->value;
			}

			if (valCnt > 1)
				val /= static_cast<double>(valCnt);

			double est = (1 - mParameters.curWeight) * mStored_IG.rbegin()->value + mParameters.curWeight * (val + mParameters.baseAvgOffset);

			// emit just the weighted average and current value
			{
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = scgms::signal_Virtual[55];
				evt.device_id() = bases_pred::model_id;
				evt.device_time() = mCurrent_Time + Prediction_Horizon;
				evt.segment_id() = mSegment_Id;
				evt.level() = est;
				mOutput.Send(evt);
			}

			const double choContribFactor =  (1.0 + mParameters.carbContrib * Calc_COB_At(mCurrent_Time + mParameters.carbPast));
			const double insContribFactor = -(1.0 + mParameters.insContrib  * Calc_IOB_At(mCurrent_Time + mParameters.insPast) );
			const double paChoContribFactor = (1.0 + mParameters.paCarbContrib * (0.5*std::tanh(10*(mLast_Physical_Activity_Index-0.3)) + 0.5));
			const double paInsContribFactor = (1.0 + mParameters.paInsContrib * (std::exp(-200*std::pow(mLast_Physical_Activity_Index-0.2, 2.0))));

			double basisFunctionContrib = 0;

			for (auto itr = mActive_Bases.begin(); itr != mActive_Bases.end(); )
			{
				const auto& fnc = *itr;

				double baseContribFactor = choContribFactor * paChoContribFactor;
				
				if (fnc.idx >= bases_pred::Base_Functions_CHO)
					baseContribFactor = insContribFactor * paInsContribFactor;

				const auto& pars = mParameters.baseFunction[fnc.idx];
				double tval = pars.amplitude * std::exp(-std::pow(mCurrent_Time + Prediction_Horizon - fnc.toff, 2.0) / (2 * pars.variance * pars.variance));

				if (tval * baseContribFactor < 0.001 && fnc.toff < mCurrent_Time)
					itr = mActive_Bases.erase(itr);
				else
					++itr;

				basisFunctionContrib += tval * baseContribFactor;
			}

			est += basisFunctionContrib;

			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
			evt.signal_id() = bases_pred::ig_signal_id;
			evt.device_id() = bases_pred::model_id;
			evt.device_time() = mCurrent_Time + Prediction_Horizon;
			evt.segment_id() = mSegment_Id;
			evt.level() = est;
			mOutput.Send(evt);
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
