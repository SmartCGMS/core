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

#include "uva_padova.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/rattime.h"

#include <type_traits>
#include <cassert>
#include <cmath>
#include <iostream>

#undef max

/*************************************************
 * UVa/Padova model implementation               *
 *************************************************/

/* The origin of this implementation lies in SimGlucose project (https://github.com/jxx123/simglucose),
 * however, it was supported by the original publication doi: 10.1177/1932296813514502
 */

/*
 For possible future comparison with SimGlucose, here is the mapping of theirs dependent variable index
 with our dependend variable name:

	 0	- Qsto1
	 1	- Qsto2
	 2	- Qgut
	 3	- Gp
	 4	- Gt
	 5	- Ip
	 6	- X
	 7	- I
	 8	- XL
	 9	- Il
	 10	- Isc1
	 11	- Isc2
	 12	- Gs

	 Note that SimGlucose does not implement the glucagon subystem. Hence, to obtain SimGlucose behavior
	 non-mapped coefficents must be set to zero. This degrades SimGlucose quality between S2013 and S2008.

	 S2008 - allows just a single meal only
	 SimGlucose - S2013 without glucagon subsystem
	 S2013 - improved S2008, contains glucagon subsytem. Still, can simulate single meal a day. Nevertheless,
			 there are studies using it for multiple days across across multiple days. Perhaps, no-action 
			 period between two meals must last long enough to get the system back to a steady state.
	 T1DMS - Likely S2013 licensed by UVA to TEG
	 S2017 - allows multiple meals in a single day, despite its use for multiday scenario. Still does not
			 contain exerise.
	 DMMS  - T1DMS successor, T1D, T2D, pre-DM, allows exercise and illness, not publicely documented

*/

CUVA_Padova_S2013_Discrete_Model::CUVA_Padova_S2013_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output) :
	CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<uva_padova_S2013::TParameters>(parameters, uva_padova_S2013::default_parameters.vector)),
	mEquation_Binding{
		{ mState.Gp,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dGp,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Gt,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dGt,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Ip,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dIp,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Il,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dIl,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Qsto1, std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dQsto1,  this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Qsto2, std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dQsto2,  this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Qgut,  std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dQgut,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.XL,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dXL,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.I,     std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dI,      this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.X,     std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dX,      this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Isc1,  std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dIsc1,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Isc2,  std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dIsc2,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Gs,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dGs,     this, std::placeholders::_1, std::placeholders::_2) },
		// not present in SimGlucose:
		{ mState.H,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dH,       this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.XH,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dXH,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.SRHS,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dSRHS, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Hsc1,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dHsc1, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Hsc2,    std::bind<double>(&CUVA_Padova_S2013_Discrete_Model::eq_dHsc2, this, std::placeholders::_1, std::placeholders::_2) },
	}
{
	mState.lastTime = -1;
	mState.Gp = mParameters.Gp_0;
	mState.Gt = mParameters.Gt_0;
	mState.Ip = mParameters.Ip_0;
	mState.Il = mParameters.Il_0;
	mState.Qsto1 = mParameters.Qsto1_0;
	mState.Qsto2 = mParameters.Qsto2_0;
	mState.Qgut = mParameters.Qgut_0;
	mState.XL = mParameters.XL_0;
	mState.I = mParameters.I_0;
	mState.X = mParameters.X_0;
	mState.Isc1 = mParameters.Isc1_0;
	mState.Isc2 = mParameters.Isc2_0;
	mState.Gs = mParameters.Gs_0;
	// not present in SimGlucose:
	mState.H = mParameters.Hb;
	mState.XH = mParameters.XH_0;
	mState.SRHS = mParameters.SRHb;
	mState.Hsc1 = mParameters.Hsc1b;
	mState.Hsc2 = mParameters.Hsc2b;

	mBasal_Ext.Add_Uptake(0, std::numeric_limits<double>::infinity(), 0.0); // TODO: BasalRate0 as a parameter
}

double CUVA_Padova_S2013_Discrete_Model::eq_dGp(const double _T, const double _X) const
{
	const double Rat = mParameters.f * mParameters.kabs * mState.Qgut / mParameters.BW;
	const double EGPt = mParameters.kp1 - mParameters.kp2 * mState.Gp - mParameters.kp3 * mState.XL + mParameters.xi * mState.XH;
	const double Uiit = mParameters.Fsnc;
	const double Et = std::max(0.0, mParameters.ke1 * (mState.Gp - mParameters.ke2));

	return mState.Gp > 0 ? std::max(0.0, EGPt) + Rat - Uiit - Et - mParameters.k1 * _X + mParameters.k2 * mState.Gt : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dGt(const double _T, const double _X) const
{
	const double Vmt = mParameters.Vm0 + mParameters.Vmx * mState.X;
	const double Kmt = mParameters.Km0;
	const double Uidt = Vmt * mState.Gt / (Kmt + mState.Gt);

	return mState.Gt > 0 ? -Uidt + mParameters.k1 * mState.Gp - mParameters.k2 * _X : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dIp(const double _T, const double _X) const
{
	return mState.Ip > 0 ? -(mParameters.m2 + mParameters.m4) * _X + mParameters.m1 * mState.Il + mParameters.ka1 * mState.Isc1 + mParameters.ka2 * mState.Isc2 : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dIl(const double _T, const double _X) const
{
	return mState.Il > 0 ? -(mParameters.m1 + mParameters.m30) * _X + mParameters.m2 * mState.Ip : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dQsto1(const double _T, const double _X) const
{
	const double mealDisturbance = mMeal_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);

	return -mParameters.kmax * _X + mealDisturbance;
}

double CUVA_Padova_S2013_Discrete_Model::Get_K_gut(const double _T) const
{
	const double mealDisturbance = mMeal_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);

	double kgut = mParameters.kmax;
	const double qsto = mState.Qsto1 + mState.Qsto2;

	const double Dbar = mealDisturbance; // TODO: revisit this, SimGlucose is probably wrong in this one
	if (Dbar > 0)
	{
		// TODO: verify the origin of 'aa' and 'cc' constants - probably obtained empirically by SimGlucose
		const double aa = 5 / 2 / (1 - mParameters.b) / Dbar;
		const double cc = 5 / 2 / mParameters.d / Dbar;
		kgut = mParameters.kmin + (mParameters.kmax - mParameters.kmin) / 2 * (std::tanh(aa * (qsto - mParameters.b * Dbar)) - std::tanh(cc * (qsto - mParameters.d * Dbar)) + 2);
	}

	return kgut;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dQsto2(const double _T, const double _X) const
{
	const double kgut = Get_K_gut(_T);

	return mParameters.kmax * mState.Qsto1 - kgut * _X;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dQgut(const double _T, const double _X) const
{
	const double kgut = Get_K_gut(_T);

	return kgut * mState.Qsto2 - mParameters.kabs * _X;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dXL(const double _T, const double _X) const
{
	return -mParameters.ki * (_X - mState.I);
}

double CUVA_Padova_S2013_Discrete_Model::eq_dI(const double _T, const double _X) const
{
	const double It = mState.Ip / mParameters.Vi;

	return -mParameters.ki * (_X - It);
}

double CUVA_Padova_S2013_Discrete_Model::eq_dX(const double _T, const double _X) const
{
	const double It = mState.Ip / mParameters.Vi;

	return -mParameters.p2u * _X + mParameters.p2u * (It - mParameters.Ib);
}

double CUVA_Padova_S2013_Discrete_Model::eq_dIsc1(const double _T, const double _X) const
{
	const double bolusDisturbance = mBolus_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);
	const double basalDisturbance = mBasal_Ext.Get_Recent(_T * scgms::One_Minute);

	const double insulinDisturbance = (bolusDisturbance + basalDisturbance) * 6000 / mParameters.BW; // U/min -> pmol/kg/min

	return mState.Isc1 > 0 ? insulinDisturbance - (mParameters.ka1 + mParameters.kd) * _X : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dIsc2(const double _T, const double _X) const
{
	return mState.Isc2 > 0 ? mParameters.kd * mState.Isc1 - mParameters.ka2 * _X : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dGs(const double _T, const double _X) const
{
	return mState.Gs > 0 ? (-mParameters.ksc * _X + mParameters.ksc * mState.Gp) : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dH(const double _T, const double _X) const
{
	const double SRHD = std::max(0.0, -eq_dGp(_T, mState.Gp)); // original equation uses -dG(t)/dt, but since G(t) = Gp(t)/Vg, the slope is identical

	return -mParameters.n * _X + (mState.SRHS + SRHD) + mParameters.kh3 * mState.Hsc2;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dXH(const double _T, const double _X) const
{
	return -mParameters.kH * _X + mParameters.kH * std::max(0.0, mState.H - mParameters.Hb);
}

double CUVA_Padova_S2013_Discrete_Model::eq_dSRHS(const double _T, const double _X) const
{
	constexpr double Gth = 60; // hypoglycaemic threshold, constant, as suggested by https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4454102/pdf/10.1177_1932296813514502.pdf

	if (mState.Gp / mParameters.Vg >= mParameters.Gb)
		return -mParameters.rho*(_X - std::max(0.0, mParameters.sigma2 * (Gth - mState.Gp / mParameters.Vg) + mParameters.SRHb));
	else
		return -mParameters.rho*(_X - std::max(0.0, mParameters.sigma1 * (Gth - mState.Gp / mParameters.Vg) / (mState.I + 1.0) + mParameters.SRHb));
}

double CUVA_Padova_S2013_Discrete_Model::eq_dHsc1(const double _T, const double _X) const
{
	return mState.Hsc1 > 0 ? -(mParameters.kh1 + mParameters.kh2) * _X : 0;
}

double CUVA_Padova_S2013_Discrete_Model::eq_dHsc2(const double _T, const double _X) const
{
	return mState.Hsc2 > 0 ? mParameters.kh1 * mState.Hsc1 - mParameters.kh3 * _X : 0;
}

void CUVA_Padova_S2013_Discrete_Model::Emit_All_Signals(double time_advance_delta)
{
	const double _T = mState.lastTime + time_advance_delta;	//locally-scoped because we might have been asked to emit the current state only

	/*
	 * Virtual insulin pump signals
	 */

	// transform requested basal rate to actually set basal rate
	if (mRequested_Basal.requested)
	{
		Emit_Signal_Level(scgms::signal_Delivered_Insulin_Basal_Rate, mRequested_Basal.time, mRequested_Basal.amount);
		mRequested_Basal.requested = false;
	}

	// transform requested bolus insulin to delivered bolus insulin amount
	for (auto& reqBolus : mRequested_Boluses)
	{
		if (reqBolus.requested) {
			Emit_Signal_Level(scgms::signal_Delivered_Insulin_Bolus, reqBolus.time, reqBolus.amount);
		}
	}
	mRequested_Boluses.clear();

	// sum of all insulin delivered by the pump
	const double dosedinsulin = (mBolus_Ext.Get_Disturbance(mState.lastTime, _T) + mBasal_Ext.Get_Recent(_T)) * (time_advance_delta / scgms::One_Minute);
	Emit_Signal_Level(uva_padova_S2013::signal_UVa_Padova_Delivered_Insulin, _T, dosedinsulin);

	/*
	 * Virtual sensor (glucometer, CGM) signals
	 */

	// BG - glucometer
	const double bglevel = scgms::mgdl_2_mmoll * (mState.Gp / mParameters.Vg);
	Emit_Signal_Level(uva_padova_S2013::signal_UVa_Padova_BG, _T, bglevel);

	// IG
	const double iglevel = scgms::mgdl_2_mmoll * (mState.Gs / mParameters.Vg);
	Emit_Signal_Level(uva_padova_S2013::signal_UVa_Padova_IG, _T, iglevel);
}

HRESULT CUVA_Padova_S2013_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {
	HRESULT res = S_FALSE;

	if (mState.lastTime > 0)
	{
		if (event.event_code() == scgms::NDevice_Event_Code::Level)
		{
			if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate)
			{
				if (event.device_time() < mState.lastTime)
					return E_ILLEGAL_STATE_CHANGE;	//got no time-machine to deliver insulin in the past
													//although we could allow this by setting it (if no newer basal is requested),
													//it would defeat the purpose of any verification

				mBasal_Ext.Add_Uptake(event.device_time(), std::numeric_limits<double>::max(), (event.level() / 60.0));
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

				// spread boluses to this much minutes
				constexpr double MinsBolusing = 1.0;

				mBolus_Ext.Add_Uptake(event.device_time(), MinsBolusing * scgms::One_Minute, (event.level() / MinsBolusing));

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
				constexpr double InvMinsEating = 1.0 / MinsEating;

				mMeal_Ext.Add_Uptake(event.device_time(), MinsEating * scgms::One_Minute, InvMinsEating * 1000.0 * event.level());
				// res = S_OK; - do not unless we have another signal called consumed CHO
			}
		}
	}

	if (res == S_FALSE)
		res = Send(event);

	return res;
}

HRESULT CUVA_Padova_S2013_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor - no need for additional configuration; signalize success
	return S_OK;
}

HRESULT IfaceCalling CUVA_Padova_S2013_Discrete_Model::Step(const double time_advance_delta) {
	HRESULT rc = E_FAIL;
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

			// Note: times in ODE solver is represented in minutes (and its fractions), as original model parameters are tuned to one minute unit
			for (auto& binding : mEquation_Binding)
				binding.x = ODE_Solver.Step(binding.fnc, nowTime / scgms::One_Minute, binding.x, microStepSize / scgms::One_Minute);
		}

		// several state variables should be non-negative, as it would mean invalid state - if the substance is depleted, then there's no way it could reach
		// negative values, if it represents a quantity
		mState.Gp = std::max(0.0, mState.Gp);
		mState.Gs = std::max(0.0, mState.Gs);
		mState.I = std::max(0.0, mState.I);
		mState.Isc1 = std::max(0.0, mState.Isc1);
		mState.Isc2 = std::max(0.0, mState.Isc2);
		mState.H = std::max(0.0, mState.H);
		mState.Hsc1 = std::max(0.0, mState.Hsc1);
		mState.Hsc2 = std::max(0.0, mState.Hsc2);

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

HRESULT CUVA_Padova_S2013_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = uva_padova_S2013::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;

	return Send(evt);
}

HRESULT IfaceCalling CUVA_Padova_S2013_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {
	if (mState.lastTime < 0.0) {
		mState.lastTime = current_time;
		mSegment_Id = segment_id;
		return S_OK;
	}
	else {
		return E_ILLEGAL_STATE_CHANGE;
	}
}
