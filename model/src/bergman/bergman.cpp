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

#ifdef WIN32
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#endif

#include <iostream>
#include <random>
#include <vector>

#include "bergman.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/FilterLib.h"

#include "ode_solvers.h"
#include "ode_solver_parameters.h"

struct meal_event {
	double t;			// device time of start
	double t_max;		// device time offset of end from start
	double amount;		// [g/min]
};

CBergman_Device_Driver::CBergman_Device_Driver(glucose::SFilter_Pipe& output)
	: mInput{}, mOutput(output) {
	//
}

CBergman_Device_Driver::~CBergman_Device_Driver() {
	if (mFeedback)
		mFeedback->Terminate_Feedback();
}

HRESULT IfaceCalling CBergman_Device_Driver::Run(glucose::IFilter_Configuration* const configuration)
{
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);

	// TODO: determine feedback type from configuration
	mFeedback = CFeedback_Receiver_Base::Create_Feedback_Receiver(NFeedback_Type::Pipe, mInput);

	if (!mFeedback->Configure_Feedback(shared_configuration))
		return E_FAIL;

	bergman_device_driver::TParameters params;

	// glucose clearance rate (insulin independent) [1/min]
	params.p1 = shared_configuration.Read_Double(rsBergman_p1, bergman_device_driver::default_parameters.p1);
	// active insulin rate of clearance (decrease of uptake) [1/min]
	params.p2 = shared_configuration.Read_Double(rsBergman_p2, bergman_device_driver::default_parameters.p2);
	// increase in update ability caused by insulin [L/(min^2 * mU)]
	params.p3 = shared_configuration.Read_Double(rsBergman_p3, bergman_device_driver::default_parameters.p3);
	// decay rate of blood insulin [1/min]
	params.p4 = shared_configuration.Read_Double(rsBergman_p4, bergman_device_driver::default_parameters.p4);

	// body weight [kg]
	params.BodyWeight = shared_configuration.Read_Double(rsBergman_BW, bergman_device_driver::default_parameters.BodyWeight);
	// insulin distribution pool volume [L]; TODO: make this relative to body weight
	params.Vi = shared_configuration.Read_Double(rsBergman_Vi, bergman_device_driver::default_parameters.Vi);
	// glucose compartment weight-dependant volume distribution [L/kg]
	params.VgDist = shared_configuration.Read_Double(rsBergman_VgDist, bergman_device_driver::default_parameters.VgDist);

	// CHO decay rate (colon to blood) [1/min]
	params.d1rate = shared_configuration.Read_Double(rsBergman_d1rate, bergman_device_driver::default_parameters.d1rate);
	// CHO absorption rate (stomach to colon) [1/min]
	params.d2rate = shared_configuration.Read_Double(rsBergman_d2rate, bergman_device_driver::default_parameters.d2rate);
	// insulin absorption rate [1/min]
	params.irate = shared_configuration.Read_Double(rsBergman_irate, bergman_device_driver::default_parameters.irate);
	// basal glucose concentration [mg/dL]
	params.Gb = shared_configuration.Read_Double(rsBergman_Gb, bergman_device_driver::default_parameters.Gb);
	// basal insulin 6concentration [mU/L]
	params.Ib = shared_configuration.Read_Double(rsBergman_Ib, bergman_device_driver::default_parameters.Ib);
	// initial glucose concentration [mg/dL]
	params.G0 = shared_configuration.Read_Double(rsBergman_G0, bergman_device_driver::default_parameters.G0);

	params.p = shared_configuration.Read_Double(rsBergman_diff2_p, bergman_device_driver::default_parameters.p);
	params.cg = shared_configuration.Read_Double(rsBergman_diff2_cg, bergman_device_driver::default_parameters.cg);
	params.c = shared_configuration.Read_Double(rsBergman_diff2_c, bergman_device_driver::default_parameters.c);

	mFeedback->Add_Interested_In_Event_Code(glucose::NDevice_Event_Code::Level);
	mFeedback->Add_Interested_In_Event_Code(glucose::NDevice_Event_Code::Information);
	mFeedback->Add_Interested_In_Signal(glucose::signal_Basal_Insulin_Rate);
	mFeedback->Add_Interested_In_Signal(glucose::signal_Calculated_Bolus_Insulin);
	mFeedback->Add_Interested_In_Signal(glucose::signal_Carb_Intake);

	mFeedback->Send_Advertisement(mOutput);

	const double Ag = 0.35;		// CHO bioavailability ratio []; this may vary with different types of meals (their glycemic index, sacharide distribution, ...)

	const double ViInv = 1 / params.Vi;	// inverse value (for calculations) of insulin distribution pool volume [1/L]
	const double Vg = params.BodyWeight * params.VgDist;	// glucose compartment volume [L]
	const double VgDLInv = 1.0 / (Vg*10.0); // inverse value (for calculations) of glucose compartment volume [1/dL]

	const double X0 = 0.0;			// initial remote insulin blood concentration [mU/L]
	const double I0 = 0.0;			// initial value of blood insulin concentration [mU/L]
	const double D10 = 0.0;			// initial value of meal disturbance [mg/dL / min]
	const double D20 = 0.0;			// initial value of CHO on board [mg/dL]
	const double Isc0 = 0.0;		// initial subcutaneous insulin amount [mU/L]
	const double Gsc0 = params.G0 - 5;		// initial value of subcutaneous glucose concentration [mg/dL]
	const double U0_basal = 0.0;	// initial exogenous insulin delivery rate [mU/min]
	const double U0_bolus = 0.0;	// initial exogenous insulin delivery rate [mU/min]

	const double dt = glucose::One_Minute * 5;			// simulation step (SmartCGMS)
	const double dtDiff = dt / glucose::One_Minute;		// transformation for parameters (fitted to "one minute unit")

	const double ode_eps0 = 0.001;	// maximum accepted error estimate for ODE solvers

	struct {
		std::vector<meal_event> meal_events;

		void Add_Meal(double t, double t_offset, double amount)
		{
			meal_events.push_back({
				t, t + t_offset, amount
			});
		}

		double Get_Meal_Disturbance(double t)
		{
			double sum = 0;
			for (auto itr = meal_events.begin(); itr != meal_events.end(); ++itr)
			{
				auto& evt = *itr;
				if (t <= evt.t_max)
					sum += evt.amount;
			}

			return sum;
		}

		void Cleanup(double t)
		{
			for (auto itr = meal_events.begin(); itr != meal_events.end(); )
			{
				auto& evt = *itr;
				if (t > evt.t_max)
					itr = meal_events.erase(itr);
				else
					itr++;
			}
		}
	} d_ext;

	double lastTime = Unix_Time_To_Rat_Time(time(nullptr));
	double G = params.G0;
	double X = X0;
	double I = I0;
	double D1 = D10;
	double D2 = D20;
	double Isc = Isc0;
	double Gsc = Gsc0;
	struct {
		double basal;
		double bolus;

		double total() {
			return basal + bolus;
		}
	} U;	// U (dosed insulin) has two parts - bolus and basal insulin, each controlled separatelly, but overall effect is the same

	U.basal = U0_basal;
	U.bolus = U0_bolus;

	// stop simulation at this time point
	// TODO: make this configurable
	const double maxTime = lastTime + glucose::One_Hour * 24.0;

	auto dG = [&](double _T, double _G) -> double {
		return -(params.p1 + X)*_G + params.p1 * params.Gb + params.d1rate * D1 * VgDLInv;
	};
	auto dX = [&](double _T, double _X) -> double {
		return -params.p2 * _X + params.p3 * (I - params.Ib);
	};
	auto dI = [&](double _T, double _I) -> double {
		return -params.p4 * _I + params.irate * Isc;
	};
	auto dIsc = [&](double _T, double _Isc) -> double {
		return -params.irate * _Isc + U.total()*ViInv;
	};
	auto dD1 = [&](double _T, double _D1) -> double {
		return -params.d1rate * _D1 + params.d2rate * D2;
	};
	auto dD2 = [&](double _T, double _D2) -> double {
		return -params.d2rate * _D2 + Ag * d_ext.Get_Meal_Disturbance(lastTime);
	};
	auto dGsc = [&](double _T, double _Gsc) -> double {
		// diffusion v2
		return (((params.p*G + params.cg * G*G + params.c) / (1.0 + params.cg * G)) - _Gsc) / dtDiff;
	};

	// helper structure for variable-equation binding
	struct eqBinding {
		double& x;
		std::function<double(double, double)> fnc;
	};

	// equation system, coupled in vector to allow generic iterative approach
	std::vector<eqBinding> equations = {
		{ G, dG }, { X, dX }, { I, dI }, { Isc, dIsc }, { D1, dD1 }, { D2, dD2 }, { Gsc, dGsc }
	};

	// "h" param for microsteps
	const double h = dtDiff;

	// helper function - sends current signal values into system
	auto sendSignals = [&]() {

		Emit_Signal_Level(glucose::signal_BG, lastTime, G * glucose::mgdl_2_mmoll);
		Emit_Signal_Level(glucose::signal_IG, lastTime, Gsc * glucose::mgdl_2_mmoll);
		Emit_Signal_Level(glucose::signal_Basal_Insulin, lastTime, 5.0 * U.total() / 1000.0);
		Emit_Signal_Level(glucose::signal_IOB, lastTime, (I + Isc) / (1000.0*ViInv));  // IOB = all insulin in system (except remote pool)
		Emit_Signal_Level(glucose::signal_Insulin_Activity, lastTime, I / (1000.0*ViInv)); // TODO: revisit this;
		Emit_Signal_Level(glucose::signal_COB, lastTime, (1.0/Ag)*(D1 + D2)/1000.0); // COB = all CHO in system (colon and stomach)
	};

	// send initial values and request synchronization
	sendSignals();

	Emit_Sync_Request(lastTime);
	lastTime += dt;

	struct FutureEvents
	{
		double amount;
		double intake_time;
	};

	std::vector<FutureEvents> futureMeals;
	std::vector<FutureEvents> futureBoluses;
	double futureBasal = 0.0;

	//ode::euler::CSolver ODE_Solver{ };
	//ode::heun::CSolver ODE_Solver{ };
	//ode::kutta::CSolver ODE_Solver{ };
	//ode::rule38::CSolver ODE_Solver{ };
	//ode::dormandprince::CSolver<> ODE_Solver{ ode_eps0 };

	ode::dormandprince::CSolver<CRunge_Kuttta_Adaptive_Strategy_Optimal_Estimation<4>> ODE_Solver{ ode_eps0 };

#define ODE_DEBUG

	for (; glucose::UDevice_Event evt = mInput.Receive(); )
	{
		if (evt.event_code == glucose::NDevice_Event_Code::Level)
		{
			if (evt.signal_id == glucose::signal_Basal_Insulin_Rate)
			{
				futureBasal = 1000.0 * (evt.level / 60.0);	// [mU/min]
			}
			else if (evt.signal_id == glucose::signal_Calculated_Bolus_Insulin)
			{
				// spread "one time bolus delivery" to 5 one-minute shots
				futureBoluses.push_back({
					1000.0 * (evt.level / 5.0),				// [mU/min]
					0
					});
			}
			else if (evt.signal_id == glucose::signal_Carb_Intake)
			{
				const double MinsEating = 10.0;
				// eating meal [mg/min]
				futureMeals.push_back({
					(1.0 / MinsEating) * 1000.0 * evt.level,
					MinsEating * glucose::One_Minute
					});
			}
		}
		else if (evt.event_code == glucose::NDevice_Event_Code::Information && evt.info == rsBergman_Feedback_Request)
		{
#ifdef ODE_DEBUG
			if (lastTime < maxTime)
#endif
			{
				// pending time event - insulin bolus
				if (!futureBoluses.empty())
				{
					for (auto& bolus : futureBoluses)
						U.bolus += bolus.amount;

					futureBoluses.clear();
				}

				// to avoid race conditions, inject new basal strictly before ODE solving
				U.basal = futureBasal;

				// pending time event - meal
				if (!futureMeals.empty())
				{
					for (auto& meal : futureMeals)
						d_ext.Add_Meal(evt.device_time, meal.intake_time, meal.amount);

					futureMeals.clear();
				}

				// perform step in all equations
				for (auto& binding : equations)
					binding.x = ODE_Solver.Step(binding.fnc, lastTime, binding.x, h);

				// emit signals and request resynchronization
				sendSignals();
				Emit_Sync_Request(lastTime);

				// cleanup history
				d_ext.Cleanup(lastTime);

				// cancel bolus after this tick (5 minutes)
				U.bolus = 0;

				lastTime += dt;
			}
#ifdef ODE_DEBUG
			else
			{
				glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Shut_Down };

				evt.device_id = bergman_device_driver::id;
				evt.segment_id = 0;

				mOutput.Send(evt);
			}
#endif
		}
		else if (evt.event_code == glucose::NDevice_Event_Code::Shut_Down || evt.event_code == glucose::NDevice_Event_Code::Synchronization)
			mOutput.Send(evt);
	}

	mFeedback->Terminate_Feedback();

	return S_OK;
}

void CBergman_Device_Driver::Emit_Signal_Level(const GUID& id, double device_time, double level)
{
	glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Level };

	evt.device_id = bergman_device_driver::id;
	evt.device_time = device_time;
	evt.level = level;
	evt.signal_id = id;
	evt.segment_id = 0;

	mOutput.Send(evt);
}

void CBergman_Device_Driver::Emit_Sync_Request(double device_time)
{
	glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Information };

	evt.device_id = bergman_device_driver::id;
	evt.device_time = device_time;
	evt.signal_id = Invalid_GUID;
	evt.segment_id = 0;
	evt.info.set(rsBergman_Feedback_Request);

	mOutput.Send(evt);
}
