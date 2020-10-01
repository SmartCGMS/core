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

#include "uva_padova_s2017.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/rattime.h"

#include <type_traits>
#include <cassert>
#include <cmath>
#include <iostream>

#undef max

// hypoglycaemic threshold, constant, as suggested by https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4454102/pdf/10.1177_1932296813514502.pdf
constexpr double Gth = 60.0;

// retrieves hour of the day (assumes unix timestamp (in double) as input, returns hour of the day with fractional part)
static double Get_Hour_Of_Day(double T)
{
	return std::modf(T * scgms::One_Minute, &T) * 24;
}

/*************************************************
 * UVa/Padova S2017 model implementation         *
 *************************************************/

CUVA_Padova_S2017_Discrete_Model::CUVA_Padova_S2017_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output) :
	CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<uva_padova_S2017::TParameters>(parameters, uva_padova_S2017::default_parameters.vector)),
	mEquation_Binding{
		{ mState.Gp,    std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dGp,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Gt,    std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dGt,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Ip,    std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dIp,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Il,    std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dIl,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Qsto1, std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dQsto1,  this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Qsto2, std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dQsto2,  this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Qgut,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dQgut,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.XL,    std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dXL,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.I,     std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dI,      this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.XH,    std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dXH,     this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.X,     std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dX,      this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Isc1,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dIsc1,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Isc2,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dIsc2,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Iid1,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dIid1,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Iid2,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dIid2,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Iih,   std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dIih,    this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Gsc,   std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dGsc,    this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.H,     std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dH,      this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.SRHS,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dSRHS,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Hsc1,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dHsc1,   this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Hsc2,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_dHsc2,   this, std::placeholders::_1, std::placeholders::_2) },
	},
	mDiffusion_Compartment_Equation_Binding{
		{ mState.idt1,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_didt1_input, this, std::placeholders::_1, std::placeholders::_2),
						std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_didt1_intermediate, this, std::placeholders::_1, std::placeholders::_2),
						std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_didt1_output, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.idt2,  std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_didt2_input, this, std::placeholders::_1, std::placeholders::_2),
						std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_didt2_intermediate, this, std::placeholders::_1, std::placeholders::_2),
						std::bind<double>(&CUVA_Padova_S2017_Discrete_Model::eq_didt2_output, this, std::placeholders::_1, std::placeholders::_2) },
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
	mState.XH = mParameters.XH_0;
	mState.X = mParameters.X_0;
	mState.Isc1 = mParameters.Isc1_0;
	mState.Isc2 = mParameters.Isc2_0;
	mState.Iid1 = mParameters.Iid1_0;
	mState.Iid2 = mParameters.Iid2_0;
	mState.Iih = mParameters.Iih_0;
	mState.Gsc = mParameters.Gsc_0;
	mState.H = mParameters.Hb;
	mState.SRHS = mParameters.SRHb;
	mState.Hsc1 = mParameters.Hsc1b;
	mState.Hsc2 = mParameters.Hsc2b;

	mState.idt1.setCompartmentCount(2); // constant, set to 2, as suggested in 10.1177/1932296818757747 and 10.1177/1932296815573864

	// TODO: when parameters event is supported, resize this according to a new value of a2
	const size_t a2 = static_cast<size_t>(mParameters.a2);
	mState.idt2.setCompartmentCount(a2 >= 2 ? a2 : 2);

	mSubcutaneous_Basal_Ext.Add_Uptake(0, std::numeric_limits<double>::infinity(), 0.0); // TODO: ScBasalRate0 as a parameter
	mIntradermal_Basal_Ext.Add_Uptake(0, std::numeric_limits<double>::infinity(), 0.0); // TODO: IdBasalRate0 as a parameter
}

double CUVA_Padova_S2017_Discrete_Model::eq_dGp(const double _T, const double _X) const
{
	const double hour = Get_Hour_Of_Day(_T);
	const double kp1t = (hour >= 3.0 && hour <= 7.0) ? mParameters.kp1 : 0.0; // apply kp1 parameter only between 3:00 AM and 7:00 AM, as the original model suggests (this *should* be redesigned so it takes a given patient's daily routine into account; e.g. sleep signals, ...)

	const double Rat = mParameters.f * mParameters.kabs * mState.Qgut / mParameters.BW;
	const double EGPt = kp1t - mParameters.kp2 * _X - mParameters.kp3 * mState.XL + mParameters.xi * mState.XH;
	const double Uiit = mParameters.Fsnc;

	const double Et = std::max(0.0, mParameters.ke1 * (_X - mParameters.ke2));

	return mState.Gp > 0 ? std::max(0.0, EGPt) + Rat - Uiit - Et - mParameters.k1 * _X + mParameters.k2 * mState.Gt : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dGt(const double _T, const double _X) const
{
	const double hour = Get_Hour_Of_Day(_T);
	const double kirt = (hour >= 3.0 && hour <= 7.0) ? mParameters.kir : 1.0; // (see note in eq_dGp)

	const double G = mState.Gp / mParameters.Vg;
	const double risk = (G > mParameters.Gb) ? ( 10*(pow(log((G > Gth) ? G : Gth), mParameters.r2) - pow(log(mParameters.Gb), mParameters.r2))) : 0;
	const double Uidt = kirt * (mParameters.Vm0 + mParameters.Vmx * mState.X * (1 + mParameters.r1*risk)) * mState.Gt / (mParameters.Km0 + mState.Gt);

	return mState.Gt > 0 ? -Uidt + mParameters.k1 * mState.Gp - mParameters.k2 * _X : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dIp(const double _T, const double _X) const
{
	const double idt1 = mState.idt1.output(); // 10.1177/1932296818757747 and 10.1177/1932296815573864

	const double RaIsc = mParameters.ka1 * mState.Isc1 + mParameters.ka2 * mState.Isc2;
	const double RaIid = idt1 * mParameters.b1 + mParameters.ka * mState.Iid2;
	const double RaIih = mParameters.kaIih * mState.Iih;
	const double RaI = RaIsc + RaIid + RaIih;

	return _X > 0 ? -(mParameters.m2 + mParameters.m4) * _X + mParameters.m1 * mState.Il + RaI : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dIl(const double _T, const double _X) const
{
	return mState.Il > 0 ? -(mParameters.m1 + mParameters.m3) * _X + mParameters.m2 * mState.Ip : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dQsto1(const double _T, const double _X) const
{
	const double mealDisturbance = mMeal_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);

	return -mParameters.kmax * _X + mealDisturbance;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dQsto2(const double _T, const double _X) const
{
	const double kempt = Get_K_empt(_T);

	return mParameters.kmax * mState.Qsto1 - kempt * _X;
}

double CUVA_Padova_S2017_Discrete_Model::Get_K_empt(const double _T) const
{
	double kempt = mParameters.kmax;

	const double Dbar = mMeal_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);
	if (Dbar > 0)
	{
		const double qsto = mState.Qsto1 + mState.Qsto2;
		kempt = mParameters.kmin + (mParameters.kmax - mParameters.kmin) / 2 * (std::tanh(mParameters.alpha * (qsto - mParameters.beta * Dbar)) - std::tanh(mParameters.beta * (qsto - mParameters.c * Dbar)) + 2);
	}

	return kempt;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dQgut(const double _T, const double _X) const
{
	const double kempt = Get_K_empt(_T);

	return kempt * mState.Qsto2 - mParameters.kabs * _X;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dXL(const double _T, const double _X) const
{
	return -mParameters.ki * (_X - mState.I);
}

double CUVA_Padova_S2017_Discrete_Model::eq_dI(const double _T, const double _X) const
{
	const double It = mState.Ip / mParameters.Vi;

	return -mParameters.ki * (_X - It);
}

double CUVA_Padova_S2017_Discrete_Model::eq_dXH(const double _T, const double _X) const
{
	return -mParameters.kH * _X + mParameters.kH * std::max(0.0, mState.H - mParameters.Hb);
}

double CUVA_Padova_S2017_Discrete_Model::eq_dX(const double _T, const double _X) const
{
	const double It = mState.Ip / mParameters.Vi;

	return -mParameters.p2u * _X + mParameters.p2u * (It - mParameters.Ib);
}

double CUVA_Padova_S2017_Discrete_Model::eq_dIsc1(const double _T, const double _X) const
{
	const double bolusDisturbance = mBolus_Insulin_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute);	// U/min
	const double basalSubcutaneousDisturbance = mSubcutaneous_Basal_Ext.Get_Recent(_T * scgms::One_Minute);			// U/min
	const double insulinSubcutaneousDisturbance = (bolusDisturbance + basalSubcutaneousDisturbance) / (scgms::pmol_2_U * mParameters.BW); // U/min -> pmol/kg/min

	return mState.Isc1 > 0 ? insulinSubcutaneousDisturbance - (mParameters.ka1 + mParameters.kd) * _X : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dIsc2(const double _T, const double _X) const
{
	return mState.Isc2 > 0 ? mParameters.kd * mState.Isc1 - mParameters.ka2 * _X : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dIid1(const double _T, const double _X) const
{
	const double intradermalInsulinDisturbance = mIntradermal_Basal_Ext.Get_Recent(_T * scgms::One_Minute) / (scgms::pmol_2_U * mParameters.BW); // U/min -> pmol/kg/min

	return mState.Iid1 > 0 ? -(0.04 + mParameters.kd) * _X + intradermalInsulinDisturbance : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dIid2(const double _T, const double _X) const
{
	const double idt2 = mState.idt2.output(); // 10.1177/1932296818757747 and 10.1177/1932296815573864

	return mState.Iid2 > 0 ? -mParameters.ka * _X + mParameters.b2 * idt2 : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dIih(const double _T, const double _X) const
{
	const double inhaledInsulinDisturbance = mInhaled_Insulin_Ext.Get_Disturbance(mState.lastTime, _T * scgms::One_Minute) / (scgms::pmol_2_U * mParameters.BW); // U/min -> pmol/kg/min

	return mState.Iih > 0 ? -mParameters.kaIih * _X + mParameters.FIih * inhaledInsulinDisturbance : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dGsc(const double _T, const double _X) const
{
	const double Ts_Inv = 1.0 / mParameters.Ts;
	const double Gt = mState.Gp / mParameters.Vg;

	return mState.Gsc > 0 ? (-Ts_Inv * _X + Ts_Inv * Gt) : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dH(const double _T, const double _X) const
{
	const double SRHD = mParameters.delta * std::max(0.0, -eq_dGp(_T, mState.Gp) / mParameters.Vg);

	return -mParameters.n * _X + (mState.SRHS + SRHD) + mParameters.kh3 * mState.Hsc2;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dSRHS(const double _T, const double _X) const
{
	const double Gt = mState.Gp / mParameters.Vg;

	if (Gt >= mParameters.Gb)
		return -mParameters.rho*(_X - mParameters.SRHb);
	else
		return -mParameters.rho*(_X - std::max(0.0, mParameters.sigma * (Gth - Gt) / (mState.I + 1.0) + mParameters.SRHb));
}

double CUVA_Padova_S2017_Discrete_Model::eq_dHsc1(const double _T, const double _X) const
{
	return mState.Hsc1 > 0 ? -(mParameters.kh1 + mParameters.kh2) * _X : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_dHsc2(const double _T, const double _X) const
{
	return mState.Hsc2 > 0 ? mParameters.kh1 * mState.Hsc1 - mParameters.kh3 * _X : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_didt1_input(const double _T, const double _X) const
{
	return mState.Iid1 > 0 ? 0.04 * mState.Iid1 : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_didt1_intermediate(const double _T, const double _X) const
{
	return (mState.idt1.quantity[mState.idt1.solverStateIdx - 1] - _X) * mParameters.b1;
}

double CUVA_Padova_S2017_Discrete_Model::eq_didt1_output(const double _T, const double _X) const
{
	return (mState.idt1.quantity[mState.idt1.solverStateIdx - 1] - _X) * mParameters.b1;
}

double CUVA_Padova_S2017_Discrete_Model::eq_didt2_input(const double _T, const double _X) const
{
	return mState.Iid1 > 0 ? mParameters.kd * mState.Iid1 : 0;
}

double CUVA_Padova_S2017_Discrete_Model::eq_didt2_intermediate(const double _T, const double _X) const
{
	return (mState.idt2.quantity[mState.idt2.solverStateIdx - 1] - _X) * mParameters.b2;
}

double CUVA_Padova_S2017_Discrete_Model::eq_didt2_output(const double _T, const double _X) const
{
	return (mState.idt2.quantity[mState.idt2.solverStateIdx - 1] - _X) * mParameters.b2;
}

void CUVA_Padova_S2017_Discrete_Model::Emit_All_Signals(double time_advance_delta)
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
	const double dosedinsulin = (mBolus_Insulin_Ext.Get_Disturbance(mState.lastTime, _T) + mSubcutaneous_Basal_Ext.Get_Recent(_T) + mInhaled_Insulin_Ext.Get_Disturbance(mState.lastTime, _T) + mIntradermal_Basal_Ext.Get_Recent(_T)) * (time_advance_delta / scgms::One_Minute);
	Emit_Signal_Level(uva_padova_S2017::signal_Delivered_Insulin, _T, dosedinsulin);

	/*
	 * Physiological signals (IOB, COB)
	 */

	// TODO: emit IOB as a sum of all (non-absorbed) insulin

	const double iob = (mState.Isc1 + mState.Isc2 /*+ mState.Iid1 + mState.Iid2 + mState.Iih + mState.Ip + mState.I + mState.Il*/) * mParameters.BW * scgms::pmol_2_U; // pmol/kg --> U
	Emit_Signal_Level(uva_padova_S2017::signal_IOB, _T, iob);

	//const double cob = 0;
	// TODO: emit COB as a sum of all (non-absorbed) carbohydrates

	/*
	 * Virtual sensor (glucometer, CGM) signals
	 */

	// BG - glucometer
	const double bglevel = scgms::mgdL_2_mmolL * (mState.Gp / mParameters.Vg);
	Emit_Signal_Level(uva_padova_S2017::signal_BG, _T, bglevel);

	// IG - CGM
	const double iglevel = scgms::mgdL_2_mmolL * mState.Gsc; // strangely Gsc is already in mg/dL
	Emit_Signal_Level(uva_padova_S2017::signal_IG, _T, iglevel);
}

HRESULT CUVA_Padova_S2017_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {
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

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				mSubcutaneous_Basal_Ext.Add_Uptake(event.device_time(), std::numeric_limits<double>::max(), (event.level() / 60.0));
				if (!mRequested_Subcutaneous_Insulin_Rate.requested || event.device_time() > mRequested_Subcutaneous_Insulin_Rate.time) {
					mRequested_Subcutaneous_Insulin_Rate.amount = event.level();
					mRequested_Subcutaneous_Insulin_Rate.time = event.device_time();
					mRequested_Subcutaneous_Insulin_Rate.requested = true;
				}

				res = S_OK;
			}
			else if (event.signal_id() == scgms::signal_Requested_Insulin_Intradermal_Rate)
			{
				if (event.device_time() < mState.lastTime)
					return E_ILLEGAL_STATE_CHANGE;

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				mIntradermal_Basal_Ext.Add_Uptake(event.device_time(), std::numeric_limits<double>::max(), (event.level() / 60.0));
				if (!mRequested_Intradermal_Insulin_Rate.requested || event.device_time() > mRequested_Intradermal_Insulin_Rate.time) {
					mRequested_Intradermal_Insulin_Rate.amount = event.level();
					mRequested_Intradermal_Insulin_Rate.time = event.device_time();
					mRequested_Intradermal_Insulin_Rate.requested = true;
				}

				res = S_OK;
			}
			else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus)
			{
				if (event.device_time() < mState.lastTime) 
					return E_ILLEGAL_STATE_CHANGE;	//got no time-machine to deliver insulin in the past

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
			else if (event.signal_id() == scgms::signal_Delivered_Insulin_Inhaled)
			{
				if (event.device_time() < mState.lastTime)
					return E_ILLEGAL_STATE_CHANGE;	//got no time-machine to deliver insulin in the past

				// inhaled insulin gets inhaled within a few seconds; let's say 10 seconds for the whole inhaling process
				constexpr double MinsInhaling = 0.2;

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				mInhaled_Insulin_Ext.Add_Uptake(event.device_time(), MinsInhaling * scgms::One_Minute, (event.level() / MinsInhaling));
				// res = S_OK; - do not unless we have another signal for already inhaled insulin (to transform this delivery into)
			}
			else if ((event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue))
			{
				//TODO: got no time-machine to consume meal in the past, but still can account for the present part of it

				// we assume 10-minute eating period
				// TODO: this should be a parameter of CHO intake
				constexpr double MinsEating = 10.0;
				constexpr double InvMinsEating = 1.0 / MinsEating;

				std::unique_lock<std::mutex> lck(mStep_Mtx);

				mMeal_Ext.Add_Uptake(event.device_time(), MinsEating * scgms::One_Minute, InvMinsEating * 1000.0 * event.level());
				// res = S_OK; - do not unless we have another signal called consumed CHO
			}
		}
	}

	if (res == S_FALSE)
		res = Send(event);

	return res;
}

HRESULT CUVA_Padova_S2017_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor - no need for additional configuration; signalize success
	return S_OK;
}

HRESULT IfaceCalling CUVA_Padova_S2017_Discrete_Model::Step(const double time_advance_delta) {
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

			for (size_t i = 0; i < microStepCount; i++)
			{
				const double nowTime = mState.lastTime + static_cast<double>(i)*microStepSize;

				// Note: times in ODE solver are represented in minutes (and its fractions), as original model parameters are tuned to one minute unit

				// step particular differential equations
				for (auto& binding : mEquation_Binding)
					binding.x = ODE_Solver.Step(binding.fnc, nowTime / scgms::One_Minute, binding.x, microStepSize / scgms::One_Minute);

				// step diffusion-compartmental equation systems
				for (auto& diffBinding : mDiffusion_Compartment_Equation_Binding)
				{
					diffBinding.x.quantity[0] = ODE_Solver.Step(diffBinding.fnc_input, nowTime / scgms::One_Minute, diffBinding.x.quantity[0], microStepSize / scgms::One_Minute);

					for (diffBinding.x.solverStateIdx = 1; diffBinding.x.solverStateIdx < diffBinding.x.compartmentCount() - 1; diffBinding.x.solverStateIdx++)
						diffBinding.x.quantity[diffBinding.x.solverStateIdx] = ODE_Solver.Step(diffBinding.fnc_intermediate, nowTime / scgms::One_Minute, diffBinding.x.quantity[diffBinding.x.solverStateIdx], microStepSize / scgms::One_Minute);

					diffBinding.x.quantity[diffBinding.x.compartmentCount()-1] = ODE_Solver.Step(diffBinding.fnc_output, nowTime / scgms::One_Minute, diffBinding.x.quantity[diffBinding.x.compartmentCount()-1], microStepSize / scgms::One_Minute);
				}

				mState.lastTime += static_cast<double>(i)*microStepSize;
			}
		}

		// several state variables should be non-negative, as it would mean invalid state - if the substance is depleted, then there's no way it could reach
		// negative values, if it represents a quantity
		mState.Gp = std::max(0.0, mState.Gp);
		mState.Gsc = std::max(0.0, mState.Gsc);
		mState.I = std::max(0.0, mState.I);
		mState.Isc1 = std::max(0.0, mState.Isc1);
		mState.Isc2 = std::max(0.0, mState.Isc2);
		mState.Iid1 = std::max(0.0, mState.Iid1);
		mState.Iid2 = std::max(0.0, mState.Iid2);
		mState.Iih = std::max(0.0, mState.Iih);
		mState.H = std::max(0.0, mState.H);
		mState.Hsc1 = std::max(0.0, mState.Hsc1);
		mState.Hsc2 = std::max(0.0, mState.Hsc2);

		mState.lastTime = oldTime;

		Emit_All_Signals(time_advance_delta);

		mMeal_Ext.Cleanup(mState.lastTime);
		mBolus_Insulin_Ext.Cleanup(mState.lastTime);
		mInhaled_Insulin_Ext.Cleanup(mState.lastTime);
		mSubcutaneous_Basal_Ext.Cleanup_Not_Recent(mState.lastTime);
		mIntradermal_Basal_Ext.Cleanup_Not_Recent(mState.lastTime);

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

HRESULT CUVA_Padova_S2017_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = uva_padova_S2017::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;

	return Send(evt);
}

HRESULT IfaceCalling CUVA_Padova_S2017_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {
	if (mState.lastTime < 0.0) {
		mState.lastTime = current_time;
		mSegment_Id = segment_id;
		return S_OK;
	}
	else {
		return E_ILLEGAL_STATE_CHANGE;
	}
}
