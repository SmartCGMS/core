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
 * Univerzitni 8
 * 301 00, Pilsen
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

// TODO: this should be meal parameter (CHO intake)
constexpr double Ag = 0.95; // CHO bioavailability ratio []; this may vary with different types of meals (their glycemic index, sacharide type and distribution, ...)

/*************************************************
 * Generic uptake accumulator implementation     *
 *************************************************/

void Uptake_Accumulator::Add_Uptake(double t, double t_delta_end, double amount)
{
	push_back({ t, t + t_delta_end, amount });
}

double Uptake_Accumulator::Get_Disturbance(double t) const
{
	double sum = 0;
	for (auto itr = begin(); itr != end(); ++itr)
	{
		auto& evt = *itr;
		if (t <= evt.t_max && t >= evt.t_min)
			sum += evt.amount;
	}

	return sum;
}

double Uptake_Accumulator::Get_Recent(double t) const
{
	if (empty())
		return 0.0;

	const Uptake_Event* cur = &(*begin());
	for (auto itr = begin(); itr != end(); ++itr)
	{
		auto& evt = *itr;
		if (t >= evt.t_min && t <= evt.t_max && cur->t_min < evt.t_min)
			cur = &evt;
	}

	return cur->amount;
}

void Uptake_Accumulator::Cleanup(double t)
{
	for (auto itr = begin(); itr != end(); )
	{
		auto& evt = *itr;
		if (t > evt.t_max)
			itr = erase(itr);
		else
			itr++;
	}
}

void Uptake_Accumulator::Cleanup_Not_Recent(double t)
{
	if (empty())
		return;

	const Uptake_Event* cur = &(*begin());

	for (auto itr = begin(); itr != end(); ++itr)
	{
		auto& evt = *itr;
		if (t >= evt.t_min && t <= evt.t_max && cur->t_min < evt.t_min)
			cur = &evt;
	}

	std::vector<Uptake_Event> remains;

	for (auto itr = begin(); itr != end(); itr++)
	{
		auto& evt = *itr;
		if (&evt == cur || evt.t_min >= t)
			remains.push_back(evt);
	}

	clear();
	assign(remains.begin(), remains.end());
}

/*************************************************
 * Bergman enhanced minimal model implementation *
 *************************************************/

CBergman_Discrete_Model::CBergman_Discrete_Model(glucose::IModel_Parameter_Vector *parameters, glucose::IFilter *output) : 
	CBase_Filter(output),
	mParameters(glucose::Convert_Parameters<bergman_model::TParameters>(parameters, bergman_model::default_parameters.vector)),
	mEquation_Binding{
		{ mState.G,   std::bind<double>(&CBergman_Discrete_Model::eq_dG, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.X,   std::bind<double>(&CBergman_Discrete_Model::eq_dX, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.I,   std::bind<double>(&CBergman_Discrete_Model::eq_dI, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Isc, std::bind<double>(&CBergman_Discrete_Model::eq_dIsc, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.Gsc, std::bind<double>(&CBergman_Discrete_Model::eq_dGsc, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.D1,  std::bind<double>(&CBergman_Discrete_Model::eq_dD1, this, std::placeholders::_1, std::placeholders::_2) },
		{ mState.D2,  std::bind<double>(&CBergman_Discrete_Model::eq_dD2, this, std::placeholders::_1, std::placeholders::_2) },
	}
{
	mState.lastTime = -1;
	mState.G = mParameters.G0;
	mState.X = mParameters.X0;
	mState.I = mParameters.I0;
	mState.D1 = mParameters.D10;
	mState.D2 = mParameters.D20;
	mState.Isc = mParameters.Isc0;
	mState.Gsc = mParameters.Gsc0;

	mBasal_Ext.Add_Uptake(0, std::numeric_limits<double>::infinity(), mParameters.BasalRate0);
}

double CBergman_Discrete_Model::eq_dG(const double _T, const double _G) const
{
	return -(mParameters.p1 + mState.X)*_G + mParameters.p1 * mParameters.Gb + mParameters.d1rate * mState.D1 / (10.0 * mParameters.BodyWeight * mParameters.VgDist);
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
	return -mParameters.d2rate * _D2 + Ag * mMeal_Ext.Get_Disturbance(_T * glucose::One_Minute);
}

double CBergman_Discrete_Model::eq_dIsc(const double _T, const double _Isc) const
{
	return -mParameters.irate * _Isc + (mBasal_Ext.Get_Recent(_T * glucose::One_Minute) + mBolus_Ext.Get_Disturbance(_T * glucose::One_Minute)) / mParameters.Vi;
}

double CBergman_Discrete_Model::eq_dGsc(const double _T, const double _Gsc) const
{
	// TODO: resolve case, when _T == mState.lastTime; for now, add 0.05, which is clearly not correct
	return (((mParameters.p*mState.G + mParameters.cg * mState.G*mState.G + mParameters.c) / (1.0 + mParameters.cg * mState.G)) - _Gsc) / (0.05 + _T - mState.lastTime/glucose::One_Minute);
}

void CBergman_Discrete_Model::Emit_All_Signals(double time_advance_delta)
{
	const double _T = mState.lastTime + time_advance_delta;	//locally-scoped because we might have been asked to emit the current state only

	// blood glucose
	Emit_Signal_Level(bergman_model::signal_Bergman_BG, _T, mState.G * glucose::mgdl_2_mmoll);

	// interstitial fluid glucose
	Emit_Signal_Level(bergman_model::signal_Bergman_IG, _T, mState.Gsc * glucose::mgdl_2_mmoll);

	// dosed basal insulin - sum of all basal insulin dosed per time_advance_delta
	// TODO: this might be a bit more precise if we calculate the actual sum during ODE solving, but the basal rate is very unlikely to change within a step, so it does not matter
	Emit_Signal_Level(bergman_model::signal_Bergman_Basal_Insulin, _T, (time_advance_delta / glucose::One_Minute) * (mBasal_Ext.Get_Recent(_T) + mBolus_Ext.Get_Disturbance(_T)) / 1000.0);

	// IOB = all insulin in system (except remote pool)
	Emit_Signal_Level(bergman_model::signal_Bergman_IOB, _T, (mState.I + mState.Isc) / (1000.0/mParameters.Vi));

	// insulin activity = "derivative" of IOB - immediate insulin effect
	// TODO: revisit this
	Emit_Signal_Level(bergman_model::signal_Bergman_Insulin_Activity, _T, mState.I / (1000.0/mParameters.Vi));

	// COB = all CHO in system (colon and stomach) - divide by bioavailability, as the CHO is physically in colon/stomach, but the body discards a part of it without absorption (determined by bioavailability ratio)
	Emit_Signal_Level(bergman_model::signal_Bergman_COB, _T, (1.0 / Ag)*(mState.D1 + mState.D2) / 1000.0);
}

HRESULT CBergman_Discrete_Model::Do_Execute(glucose::UDevice_Event event) {
	if (mState.lastTime > 0)
	{
		if (event.event_code() == glucose::NDevice_Event_Code::Level)
		{
			if (event.signal_id() == glucose::signal_Requested_Basal_Insulin_Rate)
			{
				mBasal_Ext.Add_Uptake(event.device_time(), std::numeric_limits<double>::max(), 1000.0 * (event.level() / 60.0));
			}
			else if (event.signal_id() == glucose::signal_Delivered_Insulin_Bolus || event.signal_id() == glucose::signal_Requested_Insulin_Bolus)
			{
				// we assume that bolus is spread to 5-minute rate
				constexpr double MinsBolusing = 5.0;

				mBolus_Ext.Add_Uptake(event.device_time(), MinsBolusing * glucose::One_Minute, 1000.0 * (event.level() / MinsBolusing));
			}
			else if (event.signal_id() == glucose::signal_Carb_Intake)
			{
				// we assume 10-minute eating period
				// TODO: this should be a parameter of CHO intake
				constexpr double MinsEating = 10.0;

				mMeal_Ext.Add_Uptake(event.device_time(), MinsEating * glucose::One_Minute, (1.0 / MinsEating) * 1000.0 * event.level());
			}
		}
	}

	return Send(event);
}

HRESULT CBergman_Discrete_Model::Do_Configure(glucose::SFilter_Configuration configuration) {
	// configured in the constructor
	return E_NOTIMPL;
}

HRESULT IfaceCalling CBergman_Discrete_Model::Step(const double time_advance_delta) {
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

			// Note: times in ODE solver is represented in minutes (and its fractions), as original Bergman model parameters are tuned to one minute unit
			for (auto& binding : mEquation_Binding)
				binding.x = ODE_Solver.Step(binding.fnc, nowTime / glucose::One_Minute, binding.x, microStepSize / glucose::One_Minute);
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
	glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Level };

	evt.device_id() = bergman_model::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = reinterpret_cast<std::remove_reference<decltype(evt.segment_id())>::type>(this);

	return Send(evt);
}

HRESULT IfaceCalling CBergman_Discrete_Model::Set_Current_Time(const double new_current_time) {
	if (mState.lastTime < 0.0) {
		mState.lastTime = new_current_time;
		return S_OK;
	}
	else
		return E_ILLEGAL_STATE_CHANGE;
}