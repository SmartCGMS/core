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

#include "bergman.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/rattime.h"

#include <type_traits>
#include <cassert>

/*************************************************
 * Bergman enhanced minimal model implementation *
 *************************************************/

CBergman_Discrete_Model::CBergman_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output) : 
	CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<bergman_model::TParameters>(parameters, bergman_model::default_parameters.vector)),
	mEquation_Binding{
		{ mState.Q1,  std::bind<double>(&CBergman_Discrete_Model::eq_dQ1,  this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Q2,  std::bind<double>(&CBergman_Discrete_Model::eq_dQ2,  this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.X,   std::bind<double>(&CBergman_Discrete_Model::eq_dX,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.I,   std::bind<double>(&CBergman_Discrete_Model::eq_dI,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Isc, std::bind<double>(&CBergman_Discrete_Model::eq_dIsc, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Gsc, std::bind<double>(&CBergman_Discrete_Model::eq_dGsc, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.D1,  std::bind<double>(&CBergman_Discrete_Model::eq_dD1,  this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.D2,  std::bind<double>(&CBergman_Discrete_Model::eq_dD2,  this, std::placeholders::_1, std::placeholders::_2) },
	}
{
	mState.lastTime = -std::numeric_limits<decltype(mState.lastTime)>::max();
	mInitialized = false;
	mState.Q1 = mParameters.Q10;
	mState.Q2 = mParameters.Q20;
	mState.X = mParameters.X0;
	mState.I = mParameters.I0;
	mState.D1 = mParameters.D10;
	mState.D2 = mParameters.D20;
	mState.Isc = mParameters.Isc0;
	mState.Gsc = mParameters.Gsc0 * scgms::mgdL_2_mmolL;
	mDiffIG_h_Value = mState.Gsc;

	mLastBG = mState.Q1 * scgms::mgdL_2_mmolL / (10.0 * mParameters.VgDist);
	mLastIG = mState.Gsc;

	mBasal_Ext.Add_Uptake(0, std::numeric_limits<double>::infinity(), mParameters.BasalRate0);
}

double CBergman_Discrete_Model::eq_dQ1(const double _T, const double _Q1) const
{
	return -(mParameters.p1 + mParameters.k21 + mState.X)*_Q1 + mParameters.k12*mState.Q2 + mParameters.p1 * mParameters.Qb + mParameters.d1rate * mState.D1 / mParameters.BodyWeight;
}

double CBergman_Discrete_Model::eq_dQ2(const double _T, const double _Q2) const
{
	return mParameters.k21*mState.Q1 - mParameters.k12*_Q2;
}

double CBergman_Discrete_Model::eq_dX(const double _T, const double _X) const
{
	return -mParameters.p2 * _X + mParameters.p3 * (mState.I - mParameters.Ib);
}

double CBergman_Discrete_Model::eq_dI(const double _T, const double _I) const
{
	return -mParameters.p4 * _I + mParameters.irate * mState.Isc;
}

double CBergman_Discrete_Model::eq_dD1(const double _T, const double _D1) const
{
	return -mParameters.d1rate * _D1 + mParameters.d2rate * mState.D2;
}

double CBergman_Discrete_Model::eq_dD2(const double _T, const double _D2) const
{
	return -mParameters.d2rate * _D2 + mParameters.Ag * mMeal_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);
}

double CBergman_Discrete_Model::eq_dIsc(const double _T, const double _Isc) const
{
	return -mParameters.irate * _Isc + (mBasal_Ext.Get_Recent(_T * scgms::One_Minute) + mBolus_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute)) / mParameters.Vi;
}

double CBergman_Discrete_Model::eq_dGsc(const double _T, const double _Gsc) const
{
	// Ikaros game calculates Gsc as follows (left here, for now):
	//return ((mParameters.p * mLastBG + mParameters.cg * mLastBG * (mLastBG - mLastIG) + mParameters.c) - _Gsc) / (0.05 + _T - mState.lastTime / scgms::One_Minute);

	const double GscDt = (mParameters.p * mLastBG + mParameters.cg * mLastBG * (mLastBG - mLastIG) + mParameters.c);

	if (mParameters.dt == 0.0)
		return 0.0;

	double timeDelta = mParameters.dt;
	if (mParameters.k != 0.0 && mParameters.h != 0.0)
		timeDelta += mParameters.k * mLastIG * (mLastIG - mDiffIG_h_Value) / mParameters.h;

	// this basicaly yields a line approximation, which is enough - as we know the future Gsc, the adaptive-step methods gives no error, so there's no need to perform
	// steps shorter than requested step size
	return (GscDt - _Gsc) / (timeDelta / scgms::One_Minute);
}

void CBergman_Discrete_Model::Emit_All_Signals(double time_advance_delta)
{
	const double _T = mState.lastTime + time_advance_delta;	//locally-scoped because we might have been asked to emit the current state only

	if (mRequested_Basal.requested)
	{
		Emit_Signal_Level(scgms::signal_Delivered_Insulin_Basal_Rate, mRequested_Basal.time, mRequested_Basal.amount);
		mRequested_Basal.requested = false;
	}

	for (auto& reqBolus : mRequested_Boluses)
	{
		if (reqBolus.requested) {
			Emit_Signal_Level(scgms::signal_Delivered_Insulin_Bolus, reqBolus.time, reqBolus.amount);
		}
	}
	mRequested_Boluses.clear();

	// interstitial fluid glucose
	const double iglevel = mState.Gsc;
	Emit_Signal_Level(bergman_model::signal_Bergman_IG, _T, iglevel);

	// store I(t - h) for Gsc calculation
	if (mParameters.k != 0.0 && mParameters.h != 0.0)
	{
		if (mDiffIG_h_Time < 0.0)
			mDiffIG_h_Value = iglevel;
		else if (mDiffIG_h_Time + mParameters.h < _T)
		{
			const double progress = (mDiffIG_h_Time + mParameters.h - mState.lastTime) / (_T - mState.lastTime);

			mDiffIG_h_Value = (iglevel - mLastIG) * progress;
			mDiffIG_h_Time += mParameters.h;
		}
	}

	mLastIG = iglevel;

	// blood glucose
	const double bglevel = mState.Q1 * scgms::mgdL_2_mmolL / (10.0 * mParameters.VgDist);
	Emit_Signal_Level(bergman_model::signal_Bergman_BG, _T, bglevel);
	mLastBG = bglevel;

	// dosed basal insulin - sum of all basal insulin dosed per time_advance_delta
	// TODO: this might be a bit more precise if we calculate the actual sum during ODE solving, but the basal rate is very unlikely to change within a step, so it does not matter
	Emit_Signal_Level(bergman_model::signal_Bergman_Basal_Insulin, _T, (time_advance_delta / scgms::One_Minute) * (mBasal_Ext.Get_Recent(_T) + mBolus_Ext.Get_Disturbance(mState.lastTime, _T)) / 1000.0);

	// IOB = all insulin in system (except remote pool)
	Emit_Signal_Level(bergman_model::signal_Bergman_IOB, _T, (mState.I + mState.Isc) / (1000.0/mParameters.Vi));

	// insulin activity = "derivative" of IOB - immediate insulin effect
	// TODO: revisit this
	Emit_Signal_Level(bergman_model::signal_Bergman_Insulin_Activity, _T, mState.I / (1000.0/mParameters.Vi));

	// COB = all CHO in system (colon and stomach) - divide by bioavailability, as the CHO is physically in colon/stomach, but the body discards a part of it without absorption (determined by bioavailability ratio)
	Emit_Signal_Level(bergman_model::signal_Bergman_COB, _T, (1.0 / mParameters.Ag)*(mState.D1 + mState.D2) / 1000.0);
}

HRESULT CBergman_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {
	HRESULT res = S_FALSE;	//	we consume any CHO input, but transforms insulin requests to deliveries

	if (mInitialized) {

		if (event.event_code() == scgms::NDevice_Event_Code::Level)
		{
			if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate)
			{
				if (event.device_time() < mState.lastTime)
					return E_ILLEGAL_STATE_CHANGE;	//got no time-machine to deliver insulin in the past
													//although we could allow this by setting it (if no newer basal is requested),
													//it would defeat the purpose of any verification

				mBasal_Ext.Add_Uptake(event.device_time(), std::numeric_limits<double>::max(), 1000.0 * (event.level() / 60.0));
				if (!mRequested_Basal.requested || event.device_time() > mRequested_Basal.time) {
					mRequested_Basal.amount = event.level();
					mRequested_Basal.time = event.device_time();
					mRequested_Basal.requested = true;
				}

				res = S_OK;
			}
			else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus)
			{
				if (event.device_time() < mState.lastTime) 
					return E_ILLEGAL_STATE_CHANGE;	//got no time-machine to deliver insulin in the past

				// we assume that bolus is spread to 5-minute rate
				constexpr double MinsBolusing = 5.0;

				mBolus_Ext.Add_Uptake(event.device_time(), MinsBolusing * scgms::One_Minute, 1000.0 * (event.level() / MinsBolusing));				

				mRequested_Boluses.push_back({
					event.device_time(),
					event.level(),
					true
				});

				res = S_OK;
			}
			else if ((event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue))
			{
				//TODO: got no time-machine to consume meal in the past, but still can account for the present part of it

				// we assume 10-minute eating period
				// TODO: this should be a parameter of CHO intake
				constexpr double MinsEating = 10.0;

				mMeal_Ext.Add_Uptake(event.device_time(), MinsEating * scgms::One_Minute, (1.0 / MinsEating) * 1000.0 * event.level());
				// res = S_OK; - do not unless we have another signal called consumed CHO
			}
		}
	} else 
		if (event.event_code() == scgms::NDevice_Event_Code::Level) {
			if ((event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate) ||
				(event.signal_id() == scgms::signal_Requested_Insulin_Bolus) ||
				(event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue))
				res = E_ILLEGAL_STATE_CHANGE;	//cannot modify our state prior initialization!
		}

	if (res == S_FALSE)
		res = mOutput.Send(event);

	return res;
}

HRESULT CBergman_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor - no need for additional configuration; signalize success
	return S_OK;
}

HRESULT IfaceCalling CBergman_Discrete_Model::Step(const double time_advance_delta) {
	HRESULT rc = E_INVALIDARG;
	if (!mInitialized)
		return E_ILLEGAL_METHOD_CALL;

	if (time_advance_delta > 0.0) {	
		// perform a few microsteps within advancement delta
		// we expect the spacing to be 5 minutes (between IG values) +- few seconds; however, bolus, basal intake and CHO intake may vary during this time period
		// therefore, this spreads single 5min step to 5 one-minute steps in ideal case; in less-than-ideal case (spacing greater than 5 mins), this still proceeds
		// to simulate steps shorter than 5mins (as our limit for segment extraction is maximum spacing of 15 minutes), so the intakes remain more-less consistent
		// and error remain in acceptable range
		constexpr size_t microStepCount = 5;
		const double microStepSize = time_advance_delta / static_cast<double>(microStepCount);

		for (size_t i = 0; i < microStepCount; i++)
		{
			const double nowTime = mState.lastTime + static_cast<double>(i)*microStepSize;

			// Note: times in ODE solver is represented in minutes (and its fractions), as original Bergman model parameters are tuned to one minute unit
			for (auto& binding : mEquation_Binding)
				binding.x = ODE_Solver.Step(binding.fnc, nowTime / scgms::One_Minute, binding.x, microStepSize / scgms::One_Minute);
		}

		Emit_All_Signals(time_advance_delta);

		mMeal_Ext.Cleanup(mState.lastTime);
		mBolus_Ext.Cleanup(mState.lastTime);
		mBasal_Ext.Cleanup_Not_Recent(mState.lastTime);

		mState.lastTime += time_advance_delta;

		rc = S_OK;
	}
	else if (time_advance_delta == 0.0) {
		//emiting only the current state
		Emit_All_Signals(time_advance_delta);
		rc = S_OK;
	} //else we return E_FAIL

	return rc;
}

HRESULT CBergman_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = bergman_model::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;

	return mOutput.Send(evt);
}

HRESULT IfaceCalling CBergman_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {
	if (!mInitialized) {
		mState.lastTime = current_time;
		mSegment_Id = segment_id;
		mInitialized = true;
		return S_OK;
	}
	else
		return E_ILLEGAL_STATE_CHANGE;
}