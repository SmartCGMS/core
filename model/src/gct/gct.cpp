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

#include "gct.h"

#include <scgms/rtl/SolverLib.h>

/** model-wide constants **/

constexpr const double Glucose_Molar_Weight = 180.156; // [g/mol]

static inline void Ensure_Min_Value(double& target, double value) {
	target = std::max(target, value);
}

/**
 * GCT model implementation
 */

CGCT_Discrete_Model::CGCT_Discrete_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output) :
	CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<gct_model::TParameters>(parameters, gct_model::default_parameters.vector)),

	mPhysical_Activity(mCompartments[NGCT_Compartment::Physical_Activity].Create_Depot<CExternal_State_Depot>(0.0, false)) {

	// ensure basic parametric bounds - in case some unconstrained optimization algorithm takes place
	// parameters with such values would cause trouble, as signals may yield invalid values
	Ensure_Min_Value(mParameters.t_d, scgms::One_Minute);
	Ensure_Min_Value(mParameters.t_i, scgms::One_Minute);
	Ensure_Min_Value(mParameters.Q1_0, 0.0);
	Ensure_Min_Value(mParameters.Q2_0, 0.0);
	Ensure_Min_Value(mParameters.Qsc_0, 0.0);
	Ensure_Min_Value(mParameters.X_0, 0.0);
	Ensure_Min_Value(mParameters.I_0, 0.0);
	Ensure_Min_Value(mParameters.Vq, 0.0);
	Ensure_Min_Value(mParameters.Vqsc, 0.0);
	Ensure_Min_Value(mParameters.Vi, 0.0);

	// base depots
	auto& q1 =  mCompartments[NGCT_Compartment::Glucose_1].Create_Depot(mParameters.Q1_0, false);
	auto& q2 =  mCompartments[NGCT_Compartment::Glucose_2].Create_Depot(mParameters.Q2_0, false);
	auto& qsc = mCompartments[NGCT_Compartment::Glucose_Subcutaneous].Create_Depot(mParameters.Qsc_0, false);
	auto& i =  mCompartments[NGCT_Compartment::Insulin_Base].Create_Depot(mParameters.I_0, false);
	auto& x =  mCompartments[NGCT_Compartment::Insulin_Remote].Create_Depot(mParameters.X_0, false);

	// glucose peripheral depots
	auto& q_src  = mCompartments[NGCT_Compartment::Glucose_Peripheral].Create_Depot<CSource_Depot>(mParameters.Q1b, false);
	auto& q_sink = mCompartments[NGCT_Compartment::Glucose_Peripheral].Create_Depot<CSink_Depot>(mParameters.Q1b, false);
	auto& q_moderated_sink = mCompartments[NGCT_Compartment::Glucose_Peripheral].Create_Depot<CSink_Depot>(0.0, false);

	// insulin peripheral depots
	auto& i_sink = mCompartments[NGCT_Compartment::Insulin_Peripheral].Create_Depot<CSink_Depot>(0.0, false);
	auto& i_src = mCompartments[NGCT_Compartment::Insulin_Peripheral].Create_Depot<CSource_Depot>(1.0, false); // use 1.0 as "unit amount" (is further multiplied by parameter)

	// physical activity depots
	auto& emp = mCompartments[NGCT_Compartment::Physical_Activity_Glucose_Moderation].Create_Depot(0.0, false);
	auto& emu = mCompartments[NGCT_Compartment::Physical_Activity_Glucose_Moderation].Create_Depot(0.0, false);

	q1.Set_Persistent(true);
	q1.Set_Solution_Volume(mParameters.Vq);

	q2.Set_Persistent(true);
	q2.Set_Solution_Volume(mParameters.Vq);
	
	qsc.Set_Persistent(true);
	qsc.Set_Solution_Volume(mParameters.Vqsc);

	i.Set_Persistent(true);
	i.Set_Solution_Volume(mParameters.Vi);

	x.Set_Persistent(true);
	x.Set_Solution_Volume(mParameters.Vi);

	q_src.Set_Persistent(true);
	q_src.Set_Solution_Volume(mParameters.Vq);
	q_sink.Set_Persistent(true);
	q_sink.Set_Solution_Volume(mParameters.Vq);
	q_moderated_sink.Set_Persistent(true);
	q_moderated_sink.Set_Solution_Volume(mParameters.Vq);

	i_sink.Set_Persistent(true);
	i_sink.Set_Solution_Volume(mParameters.Vi);

	mPhysical_Activity.Set_Persistent(true);
	emp.Set_Persistent(true);
	emu.Set_Persistent(true);

	//// Glucose subsystem links

	// two glucose compartments diffusion flux
	q1.Link_To<CTwo_Way_Diffusion_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(q2,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		q1, q2,
		mParameters.q12);

	// diffusion between main compartment and subcutaneous tissue
	q1.Link_To<CTwo_Way_Diffusion_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(qsc,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		q1, qsc,
		mParameters.q1sc);

	// glucose appearance due to endogennous production
	q_src.Link_To<CDifference_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(q1,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		q_src, q1,
		mParameters.q1p);

	// glucose appearance due to exercise
	q_src.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(q1,
		[&emp, this](CDepot_Link& link) {
			link.Add_Moderator<CPA_Production_Moderation_Function>(emp, mParameters.q_ep);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.q1pe);

	// glucose disappearance due to basal needs
	q1.Moderated_Link_To<CDifference_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(q_moderated_sink,
		[&x, this](CDepot_Link& link) {
			link.Add_Moderator<CLinear_Moderation_Linear_Elimination_Function>(x, mParameters.xq1, mParameters.xe);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		q1, q_moderated_sink,
		mParameters.q1e);

	// glucose disappearance due to exercise
	q1.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(q_moderated_sink,
		[&emu, this](CDepot_Link& link) {
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(emu, mParameters.q_eu);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.q1ee);

	// glucose disappearance over certain threshold (glycosuria)
	q1.Link_To<CConcentration_Threshold_Disappearance_Unbounded_Transfer_Function, double, double, IQuantizable&, double, double>(q_sink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		q1,
		mParameters.Gthr,
		mParameters.q1e_thr);

	//// Insulin subsystem links

	// available insulin to remote insulin pool
	i.Link_To<CConstant_Unbounded_Transfer_Function>(x,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ix);

	i_src.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(i,
		[&q1, this](CDepot_Link& link) {
			link.Add_Moderator<CThreshold_Linear_Moderation_No_Elimination_Function>(q1, mParameters.ip, mParameters.GIthr);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ip);

	//// Physical activity subsystem links

	// appearance of virtual "production modulator"
	mPhysical_Activity.Link_To<CDifference_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(emp,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mPhysical_Activity, emp,
		mParameters.e_pa);

	// appearance of virtual "utilization modulator"
	mPhysical_Activity.Link_To<CDifference_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(emu,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mPhysical_Activity, emu,
		mParameters.e_ua);

	// elimination rate of virtual "production modulator"
	emp.Link_To<CDifference_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(mPhysical_Activity,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		emp, mPhysical_Activity,
		mParameters.e_pe);

	// elimination rate of virtual "utilization modulator"
	emu.Link_To<CDifference_Unbounded_Transfer_Function, double, double, IQuantizable&, IQuantizable&, double>(mPhysical_Activity,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		emu, mPhysical_Activity,
		mParameters.e_ue);

	/*

	GCT model in this version does not contain:

		- inhibitors
			* glucose utilization (insulin effect inhibition)
			* glucose production (glycogen breakdown inhibition)
		- sleep effects
	
	*/
}

CDepot& CGCT_Discrete_Model::Add_To_D1(double amount, double start, double duration) {

	CDepot& depot = mCompartments[NGCT_Compartment::Carbs_1].Create_Depot(amount, false);
	CDepot& target = Add_To_D2(0, start, duration);

	depot.Link_To<CConstant_Bounded_Transfer_Function>(target, start, duration, amount);
	return depot;
}

CDepot& CGCT_Discrete_Model::Add_To_D2(double amount, double start, double duration) {

	CDepot& depot = mCompartments[NGCT_Compartment::Carbs_2].Create_Depot(amount, false);

	depot.Link_To<CConstant_Unbounded_Transfer_Function>(mCompartments[NGCT_Compartment::Glucose_1].Get_Persistent_Depot(), start, duration*10.0, mParameters.d2q1);

	return depot;
}

CDepot& CGCT_Discrete_Model::Add_To_Isc1(double amount, double start, double duration) {

	CDepot& depot = mCompartments[NGCT_Compartment::Insulin_Subcutaneous_1].Create_Depot(amount, false);
	CDepot& target = Add_To_Isc2(0, start, duration);

	depot.Link_To<CConstant_Bounded_Transfer_Function>(target, start, duration, amount);

	return depot;
}

CDepot& CGCT_Discrete_Model::Add_To_Isc2(double amount, double start, double duration) {

	CDepot& depot = mCompartments[NGCT_Compartment::Insulin_Subcutaneous_2].Create_Depot(amount, false);

	depot.Link_To<CConstant_Unbounded_Transfer_Function>(mCompartments[NGCT_Compartment::Insulin_Base].Get_Persistent_Depot(), start, duration*10.0, mParameters.ix);

	return depot;
}

void CGCT_Discrete_Model::Emit_All_Signals(double time_advance_delta) {

	const double _T = mLast_Time + time_advance_delta;

	/*
	 * Pending signals
	 */

	for (const auto& pending : mPending_Signals) {
		Emit_Signal_Level(pending.signal_id, pending.time, pending.value);
	}

	mPending_Signals.clear();

	/*
	 * Unabsorbed reservoirs (insulin/carbs on board)
	 */

	const double cob = mCompartments[NGCT_Compartment::Carbs_1].Get_Quantity() + mCompartments[NGCT_Compartment::Carbs_2].Get_Quantity(); // mmols of glucose
	Emit_Signal_Level(gct_model::signal_COB, _T, cob * Glucose_Molar_Weight / (mParameters.Ag * 1000.0));

	const double iob = mCompartments[NGCT_Compartment::Insulin_Remote].Get_Quantity();
	Emit_Signal_Level(gct_model::signal_IOB, _T, iob);

	/*
	 * Virtual sensor (glucometer, CGM) signals
	 */

	// BG - glucometer
	const double bglevel = mCompartments[NGCT_Compartment::Glucose_1].Get_Concentration();
	Emit_Signal_Level(gct_model::signal_BG, _T, bglevel);

	// IG - CGM
	const double iglevel = mCompartments[NGCT_Compartment::Glucose_Subcutaneous].Get_Concentration();
	Emit_Signal_Level(gct_model::signal_IG, _T, iglevel);
}

HRESULT CGCT_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {
	HRESULT res = S_FALSE;

	if (!std::isnan(mLast_Time)) {

		if (event.event_code() == scgms::NDevice_Event_Code::Level) {

			// insulin pump control
			if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate) {

				if (event.device_time() < mLast_Time) {
					return E_ILLEGAL_STATE_CHANGE;
				}

				mInsulin_Pump.Set_Infusion_Parameter(event.level() / 60.0, scgms::One_Minute, mParameters.t_i);
				mPending_Signals.push_back(TPending_Signal{ scgms::signal_Delivered_Insulin_Basal_Rate, event.device_time(), event.level() });

				res = S_OK;
			}
			// physical activity
			else if (event.signal_id() == scgms::signal_Physical_Activity) {

				if (event.device_time() < mLast_Time) {
					return E_ILLEGAL_STATE_CHANGE;
				}

				mPhysical_Activity.Set_Quantity(event.level());
			}
			// bolus insulin
			else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus) {

				if (event.device_time() < mLast_Time) {
					return E_ILLEGAL_STATE_CHANGE;	// got no time-machine to deliver insulin in the past
				}

				constexpr size_t Portions = 2;
				const double PortionTimeSpacing = scgms::One_Second * 30;

				const double PortionSize = event.level() / static_cast<double>(Portions);

				for (size_t i = 0; i < Portions; i++) {
					Add_To_Isc1(PortionSize, event.device_time() + static_cast<double>(i) * PortionTimeSpacing, mParameters.t_i);
				}

				mPending_Signals.push_back(TPending_Signal{ scgms::signal_Delivered_Insulin_Bolus, event.device_time(), event.level() });

				res = S_OK;
			}
			// carbs intake; NOTE: this should be further enhanced once architecture supports meal parametrization (e.g.; glycemic index, ...)
			else if ((event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue)) {

				// 10 minutes of eating - 10 portions with 1 minute spacing
				constexpr size_t Portions = 10;
				const double PortionTimeSpacing = scgms::One_Minute;

				const double PortionSize = (1000.0 * event.level() * mParameters.Ag / Glucose_Molar_Weight) / static_cast<double>(Portions);

				for (size_t i = 0; i < Portions; i++) {
					Add_To_D1(PortionSize, event.device_time() + static_cast<double>(i) * PortionTimeSpacing, mParameters.t_d);
				}

				// res = S_OK; - do not unless we have another signal called consumed CHO
			}
		}
	}

	if (res == S_FALSE) {
		res = mOutput.Send(event);
	}

	return res;
}

HRESULT CGCT_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor - no need for additional configuration; signalize success
	return S_OK;
}

HRESULT IfaceCalling CGCT_Discrete_Model::Step(const double time_advance_delta) {

	HRESULT rc = E_FAIL;
	if (time_advance_delta > 0.0) {

		constexpr size_t microStepCount = 10;
		const double microStepSize = time_advance_delta / static_cast<double>(microStepCount);
		const double oldTime = mLast_Time;
		const double futureTime = mLast_Time + time_advance_delta;

		// stepping scope
		{
			TDosage dosage;

			for (size_t i = 0; i < microStepCount; i++) {

				// for each step, retrieve insulin pump subcutaneous injection if any
				while (mInsulin_Pump.Get_Dosage(mLast_Time, dosage)) {
					Add_To_Isc1(dosage.amount, dosage.start, dosage.duration);
				}

				// step all compartments
				std::for_each(std::execution::par_unseq, mCompartments.begin(), mCompartments.end(), [this](CCompartment& comp) {
					comp.Step(mLast_Time);
				});

				// commit all compartments
				std::for_each(std::execution::par_unseq, mCompartments.begin(), mCompartments.end(), [this](CCompartment& comp) {
					comp.Commit(mLast_Time);
				});

				mLast_Time = oldTime + static_cast<double>(i) * microStepSize;
			}
		}

		mLast_Time = oldTime;

		Emit_All_Signals(time_advance_delta);

		mLast_Time = futureTime; // to avoid precision loss

		rc = S_OK;
	}
	else if (time_advance_delta == 0.0) {

		// emit the current state
		Emit_All_Signals(time_advance_delta);
		rc = S_OK;
	}

	return rc;
}

HRESULT CGCT_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = samadi_model::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;

	return mOutput.Send(evt);
}

HRESULT IfaceCalling CGCT_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {

	if (std::isnan(mLast_Time)) {

		mLast_Time = current_time;
		mSegment_Id = segment_id;

		// this is a subject of future re-evaluation - how to consider initial conditions for food-related patient state

		if (mParameters.D1_0 > 0) {
			Add_To_D1(mParameters.D1_0, mLast_Time, scgms::One_Minute * 15);
		}
		if (mParameters.D2_0 > 0) {
			Add_To_D2(mParameters.D2_0, mLast_Time, scgms::One_Minute * 15);
		}
		if (mParameters.Isc_0 > 0) {
			Add_To_Isc1(mParameters.Isc_0, mLast_Time, scgms::One_Minute * 15);
		}

		mInsulin_Pump.Initialize(mLast_Time, 0.0, 0.0, 0.0);

		return S_OK;
	}
	else {
		return E_ILLEGAL_STATE_CHANGE;
	}
}

void CInfusion_Device::Initialize(double initialTime, double infusionRate, double infusionPeriod, double absorptionTime) {

	mLast_Time = initialTime;
	mInfusion_Rate = infusionRate;
	mInfusion_Period = infusionPeriod;
	mAbsorption_Time = absorptionTime;
}

void CInfusion_Device::Set_Infusion_Parameter(double infusionRate, double infusionPeriod, double absorptionTime) {

	if (!std::isnan(infusionRate)) {
		mInfusion_Rate = infusionRate;
	}

	if (!std::isnan(infusionPeriod)) {
		mInfusion_Period = infusionPeriod;
	}

	if (!std::isnan(absorptionTime)) {
		mAbsorption_Time = absorptionTime;
	}
}

bool CInfusion_Device::Get_Dosage(double currentTime, TDosage& dosage) {

	// not yet a time for another injection
	if (currentTime < mLast_Time + mInfusion_Period) {
		return false;
	}

	// we always set zero explicitly, so it's okay to compare it like that
	if (mInfusion_Rate == 0.0) {
		return false;
	}

	mLast_Time += mInfusion_Period;
	dosage.amount = mInfusion_Rate;
	dosage.start = mLast_Time;
	dosage.duration = mAbsorption_Time;
	return true;
}
