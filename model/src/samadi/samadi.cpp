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

#include "samadi.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/rattime.h"

#include "../../../../common/utils/DebugHelper.h"

#include <type_traits>
#include <cassert>
#include <cmath>
#include <iostream>

#undef max

/** model-wide constants **/

constexpr const double GlucoseMolWeight = 180.156; // [g/mol]

/*************************************************
 * Samadi model implementation                   *
 *************************************************/

CSamadi_Discrete_Model::CSamadi_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output) :
	CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<samadi_model::TParameters>(parameters, samadi_model::default_parameters.vector)),
	mEquation_Binding{
		{ mState.Q1,    std::bind<double>(&CSamadi_Discrete_Model::eq_dQ1,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Q2,    std::bind<double>(&CSamadi_Discrete_Model::eq_dQ2,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Gsub,  std::bind<double>(&CSamadi_Discrete_Model::eq_dGsub,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.S1,    std::bind<double>(&CSamadi_Discrete_Model::eq_dS1,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.S2,    std::bind<double>(&CSamadi_Discrete_Model::eq_dS2,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.I,     std::bind<double>(&CSamadi_Discrete_Model::eq_dI,      this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.x1,    std::bind<double>(&CSamadi_Discrete_Model::eq_dx1,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.x2,    std::bind<double>(&CSamadi_Discrete_Model::eq_dx2,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.x3,    std::bind<double>(&CSamadi_Discrete_Model::eq_dx3,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.D1,    std::bind<double>(&CSamadi_Discrete_Model::eq_dD1,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.D2,    std::bind<double>(&CSamadi_Discrete_Model::eq_dD2,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.DH1,   std::bind<double>(&CSamadi_Discrete_Model::eq_dDH1,    this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.DH2,   std::bind<double>(&CSamadi_Discrete_Model::eq_dDH2,    this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.E1,    std::bind<double>(&CSamadi_Discrete_Model::eq_dE1,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.E2,    std::bind<double>(&CSamadi_Discrete_Model::eq_dE2,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.TE,    std::bind<double>(&CSamadi_Discrete_Model::eq_dTE,     this, std::placeholders::_1, std::placeholders::_2) },
	}
{
	mState.lastTime = -std::numeric_limits<decltype(mState.lastTime)>::max();
	mInitialized = false;
	mState.Q1   = mParameters.Q1_0;
	mState.Q2   = mParameters.Q2_0;
	mState.Gsub = mParameters.Gsub_0;
	mState.S1   = mParameters.S1_0;
	mState.S2   = mParameters.S2_0;
	mState.I    = mParameters.I_0;
	mState.x1   = mParameters.x1_0;
	mState.x2   = mParameters.x2_0;
	mState.x3   = mParameters.x3_0;
	mState.D1   = mParameters.D1_0;
	mState.D2   = mParameters.D2_0;
	mState.DH1  = mParameters.DH1_0;
	mState.DH2  = mParameters.DH2_0;
	mState.E1   = mParameters.E1_0;
	mState.E2   = mParameters.E2_0;
	mState.TE   = mParameters.TE_0;

	mSubcutaneous_Basal_Ext.Add_Uptake(0, std::numeric_limits<double>::max(), 0.0); // TODO: ScBasalRate0 as a parameter
	mHeart_Rate.Add_Uptake(0, std::numeric_limits<double>::max(), mParameters.HRbase);
}

double CSamadi_Discrete_Model::eq_dQ1(const double _T, const double _X) const
{
	const double VgBW = mParameters.Vg * mParameters.BW;
	const double EGP0BW = mParameters.EGP_0 * mParameters.BW;

	const double Gt = _X / VgBW;

	constexpr double Gthresh_F01 = 4.5;
	constexpr double Gthresh_FR = 9.0;

	const double F01S = mParameters.F01 * mParameters.BW;
	const double F01C = Gt >= Gthresh_F01 ? F01S : (F01S * Gt / Gthresh_F01);
	const double FR = Gt >= Gthresh_FR ? 0.003 * (Gt - 9.0) * VgBW : 0;
	const double UG = mState.D2 / mParameters.tmaxG +mState.DH2 / (mParameters.tmaxG / 2.0);

	const double ret = -(1 + mParameters.alpha * mState.E2 * mState.E2) * mState.x1 * _X + mParameters.k12 * mState.Q2 - F01C - FR + UG + EGP0BW * (1 - mState.x3);

	return ret;
}

double CSamadi_Discrete_Model::eq_dQ2(const double _T, const double _X) const
{
	return -(-1 + mParameters.alpha * mState.E2 * mState.E2) * mState.x1 * mState.Q1 - mParameters.k12 * _X - mState.x2 * _X * (1 + mParameters.alpha * mState.E2 * mState.E2 - mParameters.beta * mState.E1 / mParameters.HRbase);
}

double CSamadi_Discrete_Model::eq_dGsub(const double _T, const double _X) const
{
	const double VgBW = mParameters.Vg * mParameters.BW;

	return (1.0 / mParameters.tau_g) * ((mState.Q1 / VgBW) - _X);
}

double CSamadi_Discrete_Model::eq_dS1(const double _T, const double _X) const
{
	const double bolusDisturbance = mBolus_Insulin_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);	// U/min
	const double basalSubcutaneousDisturbance = mSubcutaneous_Basal_Ext.Get_Recent(_T * scgms::One_Minute);			// U/min
	const double insulinSubcutaneousDisturbance = (bolusDisturbance + basalSubcutaneousDisturbance) * 1000;			// U/min -> mU/min

	return insulinSubcutaneousDisturbance - _X / mParameters.tmaxi;
}

double CSamadi_Discrete_Model::eq_dS2(const double _T, const double _X) const
{
	return mState.S1 / mParameters.tmaxi - _X / mParameters.tmaxi;
}

double CSamadi_Discrete_Model::eq_dI(const double _T, const double _X) const
{
	const double ViBW = mParameters.Vi * mParameters.BW;

	return mState.S2 / (ViBW * mParameters.tmaxi) -mParameters.ke * _X;
}

double CSamadi_Discrete_Model::eq_dx1(const double _T, const double _X) const
{
	return -mParameters.ka1 * _X + mParameters.kb1 * mState.I;
}

double CSamadi_Discrete_Model::eq_dx2(const double _T, const double _X) const
{
	return -mParameters.ka2 * _X + mParameters.kb2 * mState.I;
}

double CSamadi_Discrete_Model::eq_dx3(const double _T, const double _X) const
{
	return -mParameters.ka3 * _X + mParameters.kb3 * mState.I;
}

double CSamadi_Discrete_Model::eq_dD1(const double _T, const double _X) const
{
	const double mealDisturbance = mMeal_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute) / GlucoseMolWeight;

	return mParameters.Ag * mealDisturbance - _X / mParameters.tmaxG;
}

double CSamadi_Discrete_Model::eq_dD2(const double _T, const double _X) const
{
	return mState.D1 / mParameters.tmaxG - _X / mParameters.tmaxG;
}

double CSamadi_Discrete_Model::eq_dDH1(const double _T, const double _X) const
{
	const double mealDisturbance = mRescue_Meal_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);

	return mParameters.Ag* mealDisturbance - _X / (mParameters.tmaxG / 2.0);
}

double CSamadi_Discrete_Model::eq_dDH2(const double _T, const double _X) const
{
	return mState.DH1 / (mParameters.tmaxG / 2.0) - _X / (mParameters.tmaxG / 2.0);
}

double CSamadi_Discrete_Model::eq_dE1(const double _T, const double _X) const
{
	const double HRdelta = mParameters.HRbase - mHeart_Rate.Get_Recent(_T * scgms::One_Minute);

	const double ret = (1.0 / mParameters.t_HR)* (HRdelta - _X);

	return std::isnan(ret) ? 0 : ret;
}

double CSamadi_Discrete_Model::eq_dE2(const double _T, const double _X) const
{
	const double e1fact = std::pow(mState.E1 / (mParameters.a * mParameters.HRbase), mParameters.n);
	const double fE1 = e1fact / (1 + e1fact);

	const double ret = -((fE1 / mParameters.t_in) + (1.0 / mState.TE)) * _X + fE1 * mState.TE / (mParameters.c1 + mParameters.c2);

	return std::isnan(ret) ? 0 : ret;
}

double CSamadi_Discrete_Model::eq_dTE(const double _T, const double _X) const
{
	const double e1fact = std::pow(mState.E1 / (mParameters.a * mParameters.HRbase), mParameters.n);
	const double fE1 = e1fact / (1 + e1fact);

	const double ret = (1.0 / mParameters.t_ex)* (mParameters.c1 * fE1 + mParameters.c2 - _X);
	return std::isnan(ret) ? 0 : ret;
}

void CSamadi_Discrete_Model::Emit_All_Signals(double time_advance_delta)
{
	const double _T = mState.lastTime + time_advance_delta;	// locally-scoped because we might have been asked to emit the current state only

	/*
	 * Virtual insulin pump signals
	 */

	// transform requested basal rate to actually set basal rate
	if (mRequested_Subcutaneous_Insulin_Rate.requested)
	{
		Emit_Signal_Level(scgms::signal_Delivered_Insulin_Basal_Rate, mRequested_Subcutaneous_Insulin_Rate.time, mRequested_Subcutaneous_Insulin_Rate.amount);
		mRequested_Subcutaneous_Insulin_Rate.requested = false;
	}

	// transform requested intradermal rate to actually set basal rate
	if (mRequested_Intradermal_Insulin_Rate.requested)
	{
		Emit_Signal_Level(scgms::signal_Delivered_Insulin_Intradermal_Rate, mRequested_Intradermal_Insulin_Rate.time, mRequested_Intradermal_Insulin_Rate.amount);
		mRequested_Intradermal_Insulin_Rate.requested = false;
	}

	// transform requested bolus insulin to delivered bolus insulin amount
	for (auto& reqBolus : mRequested_Insulin_Boluses)
	{
		if (reqBolus.requested) {
			Emit_Signal_Level(scgms::signal_Delivered_Insulin_Bolus, reqBolus.time, reqBolus.amount);
		}
	}
	mRequested_Insulin_Boluses.clear();

	// sum of all insulin delivered by the pump
	const double dosedinsulin = (mBolus_Insulin_Ext.Get_Disturbance(mState.lastTime, _T) + mSubcutaneous_Basal_Ext.Get_Recent(_T)) * (time_advance_delta / scgms::One_Minute);
	Emit_Signal_Level(samadi_model::signal_Delivered_Insulin, _T, dosedinsulin);

	/*
	 * Physiological signals (IOB, COB, excercise)
	 */

	// sum of S1 and S2 should be equal to IOB [mU]; then, convert to U
	const double iob = (mState.S1 + mState.S2) / 1000.0;
	Emit_Signal_Level(samadi_model::signal_IOB, _T, iob);

	// a sum of all (non-absorbed) carbohydrates - D1, D2, DH1, DH2 [mmol]; then, convert it to grams of glucose
	const double cob = (mState.D1 + mState.D2 + mState.DH1 + mState.DH2) /*[mmol]*/ * GlucoseMolWeight /*[g/mol]*/ / 1000.0;
	Emit_Signal_Level(samadi_model::signal_COB, _T, cob);

	//const double excerciseFactor = 0;
	// TODO: emit excercise factor (?) from E1/E2 compartments, maybe as a ratio of steady and current BPM?

	/*
	 * Virtual sensor (glucometer, CGM) signals
	 */

	// BG - glucometer
	const double VgBW = mParameters.Vg * mParameters.BW;
	const double bglevel = mState.Q1 / VgBW;
	Emit_Signal_Level(samadi_model::signal_BG, _T, bglevel);

	// IG - CGM
	const double iglevel = mState.Gsub;
	Emit_Signal_Level(samadi_model::signal_IG, _T, iglevel);
}

HRESULT CSamadi_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {
	HRESULT res = S_FALSE;

	if (mInitialized) {

		if (event.event_code() == scgms::NDevice_Event_Code::Level)
		{
			if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate)
			{
				if (event.device_time() < mState.lastTime)
					return E_ILLEGAL_STATE_CHANGE;	//got no time-machine to deliver insulin in the past
													//although we could allow this by setting it (if no newer basal is requested),
													//it would defeat the purpose of any verification

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				mSubcutaneous_Basal_Ext.Add_Uptake(event.device_time(), std::numeric_limits<double>::max(), (event.level() / 60.0));
				if (!mRequested_Subcutaneous_Insulin_Rate.requested || event.device_time() > mRequested_Subcutaneous_Insulin_Rate.time) {
					mRequested_Subcutaneous_Insulin_Rate.amount = event.level();
					mRequested_Subcutaneous_Insulin_Rate.time = event.device_time();
					mRequested_Subcutaneous_Insulin_Rate.requested = true;
				}

				res = S_OK;
			}
			else if (event.signal_id() == scgms::signal_Heartbeat)
			{
				if (event.device_time() < mState.lastTime)
					return E_ILLEGAL_STATE_CHANGE;

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				// TODO: is 10 minutes interval correct? This means, that if no heart rate comes in 10 minutes, we take it as "no heart rate info" and let the excercise submodel just slowly fade to its mean level
				mHeart_Rate.Add_Uptake(event.device_time(), event.device_time() + scgms::One_Minute * 10, event.level());
			}
			else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus)
			{
				if (event.device_time() < mState.lastTime) 
					return E_ILLEGAL_STATE_CHANGE;	// got no time-machine to deliver insulin in the past

				// spread boluses to this much minutes
				constexpr double MinsBolusing = 1.0;

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				mBolus_Insulin_Ext.Add_Uptake(event.device_time(), MinsBolusing * scgms::One_Minute, (event.level() / MinsBolusing));

				mRequested_Insulin_Boluses.push_back({
					event.device_time(),
					event.level(),
					true
				});

				res = S_OK;
			}
			else if ((event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue))
			{
				// TODO: got no time-machine to consume meal in the past, but still can account for the present part of it

				// we assume 10-minute eating period
				// TODO: this should be a parameter of CHO intake
				constexpr double MinsEating = 10.0;
				constexpr double InvMinsEating = 1.0 / MinsEating;

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				// Samadi/Hovorka model differentiates between regular and rescue CHO

				if (event.signal_id() == scgms::signal_Carb_Intake)
					mMeal_Ext.Add_Uptake(event.device_time(), MinsEating * scgms::One_Minute, InvMinsEating * 1000.0 * event.level());
				else if (event.signal_id() == scgms::signal_Carb_Rescue)
					mRescue_Meal_Ext.Add_Uptake(event.device_time(), MinsEating * scgms::One_Minute, InvMinsEating * 1000.0 * event.level());

				// res = S_OK; - do not unless we have another signal called consumed CHO
			}
		}
	}
	else {
		if (event.event_code() == scgms::NDevice_Event_Code::Level) {
			if ((event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate) ||
				(event.signal_id() == scgms::signal_Requested_Insulin_Bolus) ||
				(event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue))
				res = E_ILLEGAL_STATE_CHANGE;	//cannot modify our state prior initialization!
		}
	}

	if (res == S_FALSE)
		res = mOutput.Send(event);

	return res;
}

HRESULT CSamadi_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor - no need for additional configuration; signalize success
	return S_OK;
}

HRESULT IfaceCalling CSamadi_Discrete_Model::Step(const double time_advance_delta) {

	if (!mInitialized)
		return E_ILLEGAL_METHOD_CALL;

	HRESULT rc = E_FAIL;
	if (time_advance_delta > 0.0) {
		// perform a few microsteps within advancement delta
		// we expect the spacing to be 5 minutes (between IG values) +- few seconds; however, bolus, basal intake and CHO intake may vary during this time period
		// therefore, this spreads single 5min step to 5 one-minute steps in ideal case; in less-than-ideal case (spacing greater than 5 mins), this still proceeds
		// to simulate steps shorter than 5mins (as our limit for segment extraction is maximum spacing of 15 minutes), so the intakes remain more-less consistent
		// and error remain in acceptable range
		constexpr size_t microStepCount = 5;
		const double microStepSize = time_advance_delta / static_cast<double>(microStepCount);
		const double oldTime = mState.lastTime;
		const double futureTime = mState.lastTime + time_advance_delta;

		// lock scope
		{
			std::unique_lock<std::mutex> lck(mStep_Mtx);

			std::vector<double> next_step_values(mEquation_Binding.size());

			for (size_t i = 0; i < microStepCount; i++)
			{
				const double nowTime = mState.lastTime + static_cast<double>(i)*microStepSize;

				// Note: times in ODE solver are represented in minutes (and its fractions), as original model parameters are tuned to one minute unit

				// calculate next step
				for (size_t j = 0; j < mEquation_Binding.size(); j++)
					next_step_values[j] = ODE_Solver.Step(mEquation_Binding[j].fnc, nowTime / scgms::One_Minute, mEquation_Binding[j].x, microStepSize / scgms::One_Minute);

				// commit
				for (size_t j = 0; j < mEquation_Binding.size(); j++)
					mEquation_Binding[j].x = next_step_values[j];

				mState.lastTime += static_cast<double>(i)*microStepSize;
			}
		}

		// several state variables should be non-negative, as it would mean invalid state - if the substance is depleted, then there's no way it could reach
		// negative values, if it represents a quantity
		mState.Q1 = std::max(0.0, mState.Q1);
		mState.Q2 = std::max(0.0, mState.Q2);
		mState.Gsub = std::max(0.0, mState.Gsub);
		mState.I = std::max(0.0, mState.I);
		mState.x1 = std::max(0.0, mState.x1);
		mState.x2 = std::max(0.0, mState.x2);
		mState.x3 = std::max(0.0, mState.x3);
		mState.D1 = std::max(0.0, mState.D1);
		mState.D2 = std::max(0.0, mState.D2);
		mState.DH1 = std::max(0.0, mState.DH1);
		mState.DH2 = std::max(0.0, mState.DH2);
		mState.E1 = std::max(0.0, mState.E1);
		mState.E2 = std::max(0.0, mState.E2);
		mState.TE = std::max(0.0, mState.TE);

		mState.lastTime = oldTime;

		Emit_All_Signals(time_advance_delta);

		mMeal_Ext.Cleanup(mState.lastTime);
		mBolus_Insulin_Ext.Cleanup(mState.lastTime);
		mSubcutaneous_Basal_Ext.Cleanup_Not_Recent(mState.lastTime);

		mState.lastTime = futureTime; // to avoid precision loss

		rc = S_OK;
	}
	else if (time_advance_delta == 0.0) {
		//emiting only the current state
		Emit_All_Signals(time_advance_delta);
		rc = S_OK;
	} //else we return E_FAIL

	return rc;
}

HRESULT CSamadi_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = samadi_model::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;

	return mOutput.Send(evt);
}

HRESULT IfaceCalling CSamadi_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {
	if (!mInitialized) {
		mState.lastTime = current_time;
		mSegment_Id = segment_id;
		mInitialized = true;
		return S_OK;
	}
	else {
		return E_ILLEGAL_STATE_CHANGE;
	}
}
