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

#include "bases_standalone.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/ApproxLib.h"
#include "../../../../common/utils/math_utils.h"

#include "../../../../common/utils/DebugHelper.h"

enum class NIG_History_Mode
{
	Moving_Average,
	Median,
	Moving_Average_Plus_StdDev,
};

constexpr NIG_History_Mode IG_History_Mode = NIG_History_Mode::Moving_Average;

using namespace scgms::literals;

CBase_Functions_Standalone::CBase_Functions_Standalone(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<bases_standalone::TParameters>(parameters, bases_standalone::default_parameters.vector)) {
	//

	mTime_Seg = refcnt::Manufacture_Object_Shared<scgms::CTime_Segment, scgms::ITime_Segment, scgms::STime_Segment>();
}

void CBase_Functions_Standalone::Store_IG(double time, double level)
{
	mStored_IG.push_back({
		time, time, level
	});

	for (auto itr = mStored_IG.begin(); itr != mStored_IG.end(); )
	{
		if (itr->time_start < mCurrent_Time - mParameters.baseAvgTimeWindow)
			itr = mStored_IG.erase(itr);
		else // once we find a value that falls within the window, end the cleanup - values are always ordered from oldest to newest
			break;
	}

	double times = time;
	double levels = level;
	auto ig = mTime_Seg.Get_Signal(scgms::signal_IG);
	ig->Update_Levels(&times, &levels, 1);
}

HRESULT CBase_Functions_Standalone::Do_Execute(scgms::UDevice_Event event)
{
	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		/*if (event.signal_id() == scgms::signal_IG)
		{
			
		}
		else */if (event.signal_id() == scgms::signal_Carb_Intake)
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

			if (!Is_Any_NaN(event.level()))
				mPhysical_Activity_Decaying_Index += event.level() * (1 - mParameters.pa_decay);
		}
	}

	return mOutput.Send(event);
}

HRESULT CBase_Functions_Standalone::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	return S_OK;
}

HRESULT IfaceCalling CBase_Functions_Standalone::Initialize(const double current_time, const uint64_t segment_id)
{
	mCurrent_Time = current_time;
	mLast_Activated_Day = std::floor(mCurrent_Time);
	mSegment_Id = segment_id;

	// store initial IG
	Store_IG(current_time, mParameters.c);

	mStored_Carbs.push_back({
		current_time,
		static_cast<double>(current_time + 5.0_hr),
		mParameters.initCarbs
	});
	mStored_Insulin.push_back({
		current_time,
		static_cast<double>(current_time + 5.0_hr),
		mParameters.initIns
	});

	return S_OK;
}

HRESULT IfaceCalling CBase_Functions_Standalone::Step(const double time_advance_delta)
{
	if (time_advance_delta > 0)
	{
		mCurrent_Time += time_advance_delta;

		// decay
		mPhysical_Activity_Decaying_Index = mParameters.pa_decay * (time_advance_delta / (scgms::One_Minute * 5.0)) * (mPhysical_Activity_Decaying_Index);

		if (mCurrent_Time + 3.0 > mLast_Activated_Day)
		{
			for (size_t i = 0; i < bases_standalone::Base_Functions_Count; i++)
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

			double est = mParameters.c // constant base value
				         + (mParameters.baseValue - val)*mParameters.baseWeight             // steady state tendency
				         + (1 - mParameters.curWeight) * mStored_IG.rbegin()->value         // autoregression part 1 (weighted last value)
				         +      mParameters.curWeight  * (val + mParameters.baseAvgOffset); // autoregression part 2 (weighted value of moving average)

			if (Is_Any_NaN(mLast_Physical_Activity_Index))
				mLast_Physical_Activity_Index = 0;

			const double choContribFactor = (0.0 + mParameters.carbContrib * Calc_COB_At(mCurrent_Time - mParameters.carbPast));
			const double insContribFactor = (0.0 + mParameters.insContrib * Calc_IOB_At(mCurrent_Time - mParameters.insPast));
			const double paContribFactor = (0.0 + mParameters.paContrib * mLast_Physical_Activity_Index);

			double basisFunctionContrib = 0;
			double unfactoredBasisFunctionContrib = 0;

			double insBasesContrib = 0;
			double choBasesContrib = 0;

			for (auto itr = mActive_Bases.begin(); itr != mActive_Bases.end(); )
			{
				const auto& fnc = *itr;

				double baseContribFactor = choContribFactor;
				
				if (fnc.idx >= bases_standalone::Base_Functions_CHO + bases_standalone::Base_Functions_Ins)
					baseContribFactor = paContribFactor;
				else if (fnc.idx >= bases_standalone::Base_Functions_CHO)
					baseContribFactor = insContribFactor;
				else if (fnc.idx >= bases_standalone::Base_Functions_Count)
					continue;

				const auto& pars = mParameters.baseFunction[fnc.idx];
				double tval = pars.amplitude * std::exp(-std::pow(mCurrent_Time - fnc.toff, 2.0) / (2 * pars.variance * pars.variance));

				unfactoredBasisFunctionContrib += tval;

				/*if (fnc.toff + 24.0_hr < mCurrent_Time && tval < 0.01) // the basis function does not contribute much to the overal result - delete it
				{
					itr = mActive_Bases.erase(itr);
					continue;
				}
				else*/
					++itr;

				basisFunctionContrib += tval * baseContribFactor;
			}

			basisFunctionContrib += mParameters.h1 * std::log(std::abs(mParameters.k1 * mPhysical_Activity_Decaying_Index) + 1.0);

			// emit the basis function contribution
			/*
			{
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = scgms::signal_Steps;
				evt.device_id() = bases_standalone::model_id;
				evt.device_time() = mCurrent_Time;
				evt.segment_id() = mSegment_Id;
				evt.level() = choContribFactor *20;
				mOutput.Send(evt);
			}
			{
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = scgms::signal_Acceleration;
				evt.device_id() = bases_standalone::model_id;
				evt.device_time() = mCurrent_Time;
				evt.segment_id() = mSegment_Id;
				evt.level() = insBasesContrib;
				mOutput.Send(evt);
			}*/

			est += basisFunctionContrib;

			Store_IG(mCurrent_Time, est);

			// emit the prediction
			{
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = bases_standalone::ig_signal_id;
				evt.device_id() = bases_standalone::model_id;
				evt.device_time() = mCurrent_Time;
				evt.segment_id() = mSegment_Id;
				evt.level() = est;
				mOutput.Send(evt);
			}
		}
		else {
			Store_IG(mCurrent_Time, mParameters.c);
		}

		auto cleanup = [this](auto& cont) {
			for (auto itr = cont.begin(); itr != cont.end(); )
			{
				if (itr->time_end + 360_min < mCurrent_Time)
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

double CBase_Functions_Standalone::Calc_IOB_At(const double time)
{
	double iob_acc = 0.0;

	const double t_max = mParameters.t_maxI;
	const double Vi = mParameters.vi;
	const double ke = mParameters.ke;

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

double CBase_Functions_Standalone::Calc_COB_At(const double time)
{
	double cob_acc = 0.0;

	//C(t) = (Dg * Ag * t * e^(-t/t_max)) / t_max^2

	const double t_max = mParameters.t_maxD; //min
	const double Ag = mParameters.Ag;

	const double GlucMolar = 180.16; // glucose molar weight (g/mol)
	const double gCHO_to_mmol = (1000 / GlucMolar); // 1000 -> g to mg, 180.16 - molar weight of glucose; g -> mmol
	const double mmol_to_gCHO = 1.0 / gCHO_to_mmol;

	for (auto& r : mStored_Carbs)
	{
		if (time >= r.time_start)// && time < r.time_end)
		{
			const double t = (time - r.time_start) / scgms::One_Minute; // device_time (rat time) to minutes

			const double Dg = gCHO_to_mmol * r.value;

			cob_acc += (Dg * Ag * t * std::exp(-t / t_max)) / std::pow(t_max, 2.0);
			//cob_acc += Dg * Ag - (Dg * Ag * (t_max - std::exp(-t / t_max) * (t_max + t)) / t_max);
		}
	}

	return cob_acc * mmol_to_gCHO * 1000.0;
}
