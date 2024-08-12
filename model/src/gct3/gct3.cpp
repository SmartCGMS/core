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

#include "gct3.h"

#include <scgms/rtl/SolverLib.h>

// this selects GCTv3 support implementations
using namespace gct3_model;

/** model-wide constants **/

constexpr const double Glucose_Molar_Weight = 180.156; // [g/mol]

static inline void Ensure_Min_Value(double& target, double value) {
	target = std::max(target, value);
}

/**
 * GCTv3 model implementation
 */

CGCT3_Discrete_Model::CGCT3_Discrete_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output) :
	CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<gct3_model::TParameters>(parameters, gct3_model::default_parameters.vector)),

	mPhysical_Activity(mCompartments[NGCT_Compartment::Physical_Activity].Create_Depot<CExternal_State_Depot>(0.0, false)),
	mInsulin_Sink(mCompartments[NGCT_Compartment::Insulin_Peripheral].Create_Depot<CSink_Depot>(0.0, false)) {

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
	auto& q_sink = mCompartments[NGCT_Compartment::Glucose_Peripheral].Create_Depot<CSink_Depot>(0.0, false);

	// insulin peripheral depots
	auto& i_src  = mCompartments[NGCT_Compartment::Insulin_Peripheral].Create_Depot<CSource_Depot>(1.0, false); // use 1.0 as "unit amount" (is further multiplied by parameter)

	// physical activity depots
	auto& emp = mCompartments[NGCT_Compartment::Physical_Activity_Glucose_Moderation_Short_Term].Create_Depot(0.0, false);
	auto& emu = mCompartments[NGCT_Compartment::Physical_Activity_Glucose_Moderation_Short_Term].Create_Depot(0.0, false);
	auto& elt = mCompartments[NGCT_Compartment::Physical_Activity_Glucose_Moderation_Long_Term].Create_Depot(0.0, false);

	auto& cins = mCompartments[NGCT_Compartment::Circadian_Insulin].Create_Depot<CExternal_Circadian_State_Depot>(0.0, true);

	q1.Set_Persistent(true);
	q1.Set_Solution_Volume(mParameters.Vq);
	q1.Set_Name(L"Q1");

	q2.Set_Persistent(true);
	q2.Set_Solution_Volume(mParameters.Vq);
	q2.Set_Name(L"Q2");
	
	qsc.Set_Persistent(true);
	qsc.Set_Solution_Volume(mParameters.Vqsc);
	qsc.Set_Name(L"Qsc");

	i.Set_Persistent(true);
	i.Set_Solution_Volume(mParameters.Vi);
	i.Set_Name(L"I");

	x.Set_Persistent(true);
	x.Set_Solution_Volume(mParameters.Vi);
	x.Set_Name(L"X");

	q_src.Set_Persistent(true);
	q_src.Set_Solution_Volume(mParameters.Vq);
	q_src.Set_Name(L"Qsrc");
	q_sink.Set_Persistent(true);
	q_sink.Set_Solution_Volume(mParameters.Vq);
	q_sink.Set_Name(L"Qsink");

	i_src.Set_Persistent(true);
	i_src.Set_Name(L"Isrc");
	mInsulin_Sink.Set_Persistent(true);
	mInsulin_Sink.Set_Name(L"Isink");

	mPhysical_Activity.Set_Persistent(true);
	mPhysical_Activity.Set_Name(L"PA");
	emp.Set_Persistent(true);
	emp.Set_Name(L"Emp");
	emu.Set_Persistent(true);
	emu.Set_Name(L"Emu");
	elt.Set_Persistent(true);
	elt.Set_Name(L"Elt");

	cins.Set_Persistent(true);
	cins.Set_Name(L"Circadian Is");

	cins.Add_Knot(0.25, mParameters.ci_0);
	cins.Add_Knot(0.5, mParameters.ci_1);
	cins.Add_Knot(0.75, mParameters.ci_2);

	//// Glucose subsystem links

	// two glucose compartments diffusion flux
	q1.Link_To<CTwo_Way_Diffusion_Unbounded_Transfer_Function>(q2,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(q1), std::ref(q2),
		mParameters.q12);

	// diffusion between main compartment and subcutaneous tissue
	q1.Link_To<CTwo_Way_Diffusion_Unbounded_Transfer_Function>(qsc,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(q1), std::ref(qsc),
		mParameters.q1sc);

	// glucose appearance due to endogennous production
	q_src.Moderated_Link_To<CDifference_Unbounded_Transfer_Function>(q1,
		[&q1, &x, this](CDepot_Link& link) {
			// production is inhibited by insulin presence
			link.Add_Moderator<CGaussian_Base_Moderation_No_Elimination_Function>(x, mParameters.q1pi);
			// glucose appearance moderated by itself
			link.Add_Moderator<CLinear_Base_Moderation_No_Elimination_Function>(q1, mParameters.dqscm);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(q_src), std::ref(q1),
		mParameters.q1p);

	// glucose appearance due to exercise
	q_src.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(q1,
		[&emp, this](CDepot_Link& link) {
			link.Add_Moderator<CPA_Production_Moderation_Function>(emp, mParameters.q_ep);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.q1pe);

	// glucose elimination due to basal and peripheral needs
	q1.Moderated_Link_To<CDifference_Unbounded_Transfer_Function>(q_sink,
		[&q1, &x, &elt, &cins, this](CDepot_Link& link) {
			// glucose elimination is moderated by insulin
			link.Add_Moderator<CLinear_Moderation_Linear_Elimination_Function>(x, mParameters.xq1, mParameters.xe);
			// glucose elimination moderated by insulin and circadian response
			link.Add_Moderator<CLinear_Base_Moderation_No_Elimination_Function>(cins, 1.0);
			// glucose elimination moderated by itself
			link.Add_Moderator<CLinear_Base_Moderation_No_Elimination_Function>(q1, mParameters.iqscm);
			// insulin sensitivity change as a result of physical activity
			link.Add_Moderator<CLinear_Base_Moderation_No_Elimination_Function>(elt, mParameters.e_Si);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(q1), std::ref(q_sink),
		mParameters.q1e);

	// glucose elimination due to exercise
	q1.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(q_sink,
		[&emu, this](CDepot_Link& link) {
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(emu, mParameters.q_eu);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.q1ee);

	// glucose elimination over certain threshold (glycosuria)
	q1.Link_To<CConcentration_Threshold_Disappearance_Unbounded_Transfer_Function>(q_sink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(q1),
		mParameters.Gthr,
		mParameters.q1e_thr);

	//// Insulin subsystem links

	// available insulin to remote insulin pool
	i.Link_To<CConstant_Unbounded_Transfer_Function>(x,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ix);

	// insulin production, moderated by glucose presence in Q1 depot over certain threshold
	i_src.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(i,
		[&q1, this](CDepot_Link& link) {
			link.Add_Moderator<CThreshold_Linear_Moderation_No_Elimination_Function>(q1, 1.0, mParameters.GIthr);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ip);

	//// Physical activity subsystem links
	// mostly based on https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5872070/ and https://www.ncbi.nlm.nih.gov/pmc/articles/PMC2769951/

	// appearance of virtual "production modulator"
	mPhysical_Activity.Link_To<CDifference_Unbounded_Transfer_Function>(emp,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(mPhysical_Activity), std::ref(emp),
		mParameters.e_pa);

	// appearance of virtual "utilization modulator"
	mPhysical_Activity.Link_To<CDifference_Unbounded_Transfer_Function>(emu,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(mPhysical_Activity), std::ref(emu),
		mParameters.e_ua);

	// elimination rate of virtual "production modulator"
	emp.Link_To<CDifference_Unbounded_Transfer_Function>(mPhysical_Activity,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(emp), std::ref(mPhysical_Activity),
		mParameters.e_pe);

	// elimination rate of virtual "utilization modulator"
	emu.Link_To<CDifference_Unbounded_Transfer_Function>(mPhysical_Activity,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(emu), std::ref(mPhysical_Activity),
		mParameters.e_ue);

	// appearance of virtual "long-term modulator"
	mPhysical_Activity.Link_To<CDifference_Unbounded_Transfer_Function>(elt,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(mPhysical_Activity), std::ref(elt),
		mParameters.e_lta);

	// elimination rate of virtual "long-term modulator"
	elt.Link_To<CConstant_Unbounded_Transfer_Function>(mPhysical_Activity,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.e_lte);

	/*

	GCT model in this version does not contain:

		- inhibitors
			* glucose utilization (insulin effect inhibition)
		- sleep effects
	
	*/
}

CDepot& CGCT3_Discrete_Model::Add_To_D1(double amount, double start, double duration) {

	CDepot& depot = mCompartments[NGCT_Compartment::Carbs].Create_Depot(amount, false);

	depot.Set_Name(std::wstring(L"D1 (") + std::to_wstring(amount) + L")");

	depot.Link_To<CTriangular_Bounded_Transfer_Function>(mCompartments[NGCT_Compartment::Glucose_1].Get_Persistent_Depot(), start, duration, amount);

	return depot;
}

CDepot& CGCT3_Discrete_Model::Add_To_Isc1(double amount, double start, double duration) {

	CDepot& depot = mCompartments[NGCT_Compartment::Insulin_Subcutaneous].Create_Depot(amount * mParameters.iscimod, false);

	depot.Set_Name(std::wstring(L"Isc1 (") + std::to_wstring(amount) + L")");

	// absorbed insulin is lowered by the ratio of absorption to elimination
	depot.Link_To<CConstant_Bounded_Transfer_Function>(mCompartments[NGCT_Compartment::Insulin_Base].Get_Persistent_Depot(), start, duration, amount * mParameters.iscimod);

	return depot;
}

void CGCT3_Discrete_Model::Emit_All_Signals(double time_advance_delta) {

	const double _T = mLast_Time;

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

	const double cob = mCompartments[NGCT_Compartment::Carbs].Get_Quantity(); // mmols of glucose
	Emit_Signal_Level(gct3_model::signal_COB, _T, cob * Glucose_Molar_Weight / (mParameters.Ag * 1000.0));

	const double iob = mCompartments[NGCT_Compartment::Insulin_Remote].Get_Quantity() + mCompartments[NGCT_Compartment::Insulin_Subcutaneous].Get_Quantity();
	Emit_Signal_Level(gct3_model::signal_IOB, _T, iob);

	/*
	 * Virtual sensor (glucometer, CGM) signals
	 */

	// BG - glucometer
	const double bglevel = mCompartments[NGCT_Compartment::Glucose_1].Get_Concentration();
	Emit_Signal_Level(gct3_model::signal_BG, _T, bglevel);

	// IG - CGM
	const double iglevel = mCompartments[NGCT_Compartment::Glucose_Subcutaneous].Get_Concentration();
	Emit_Signal_Level(gct3_model::signal_IG, _T, iglevel);
}

HRESULT CGCT3_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {
	HRESULT res = S_FALSE;

	if (!std::isnan(mLast_Time)) {

		if (event.event_code() == scgms::NDevice_Event_Code::Level) {

			// insulin pump control
			if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate) {

				//if (event.device_time() >= mLast_Time)
				{
					constexpr double PortionTimeSpacing = 60_sec;
					const double rate = event.level() / (60.0 * (60_sec / PortionTimeSpacing));

					mInsulin_Pump.Set_Infusion_Parameter(event.device_time(), rate, PortionTimeSpacing, mParameters.t_i);
					mPending_Signals.push_back(TPending_Signal{ scgms::signal_Delivered_Insulin_Basal_Rate, event.device_time(), event.level() });

					res = S_OK;
				}
			}
			// physical activity
			else if (event.signal_id() == scgms::signal_Physical_Activity) {

				//if (event.device_time() >= mLast_Time) {
					mPhysical_Activity.Set_Quantity(event.level());
				//}
			}
			// bolus insulin
			else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus) {

				//if (event.device_time() >= mLast_Time)
				{
					constexpr double PortionTimeSpacing = scgms::One_Minute;
					const size_t Portions = static_cast<size_t>(mParameters.t_id / PortionTimeSpacing);

					/*
					constexpr size_t Portions = 2;
					const double PortionTimeSpacing = mParameters.t_id / static_cast<double>(Portions);
					*/

					const double PortionSize = event.level() / static_cast<double>(Portions);

					for (size_t i = 0; i < Portions; i++) {
						Add_To_Isc1(PortionSize, event.device_time() + static_cast<double>(i) * PortionTimeSpacing, mParameters.t_i);
					}

					mPending_Signals.push_back(TPending_Signal{ scgms::signal_Delivered_Insulin_Bolus, event.device_time(), event.level() });

					res = S_OK;
				}
			}
			// carbs intake; NOTE: this should be further enhanced once architecture supports meal parametrization (e.g.; glycemic index, ...)
			else if ((event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue)) {

				// TODO: figure out optimal spacing and portion size to reflect real scenarios; for now, assume 2 "big bites" with 1 minute spacing (acceptable for debugging purposes)
				constexpr size_t Portions = 10;
				constexpr double PortionTimeSpacing = 30_sec;

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

HRESULT CGCT3_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor - no need for additional configuration; signalize success
	return S_OK;
}

HRESULT IfaceCalling CGCT3_Discrete_Model::Step(const double time_advance_delta) {

	HRESULT rc = E_FAIL;
	if (time_advance_delta > 0.0) {

		constexpr size_t microStepCount = 5;

		const double microStepSize = time_advance_delta / static_cast<double>(microStepCount);
		const double oldTime = mLast_Time;
		const double futureTime = mLast_Time + time_advance_delta;

		// stepping scope
		{
			TDosage dosage;

			for (size_t i = 0; i < microStepCount; i++) {

				// for each step, retrieve insulin pump subcutaneous injection if any
				if (mInsulin_Pump.Get_Dosage(mLast_Time, dosage)) {
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

		mLast_Time = futureTime; // to avoid precision loss

		Emit_All_Signals(time_advance_delta);

		rc = S_OK;
	}
	else if (time_advance_delta == 0.0) {

		// emit the current state
		Emit_All_Signals(time_advance_delta);
		rc = S_OK;
	}

	return rc;
}

HRESULT CGCT3_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = gct3_model::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;

	return mOutput.Send(evt);
}

HRESULT IfaceCalling CGCT3_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {

	if (std::isnan(mLast_Time)) {

		mLast_Time = current_time;
		mSegment_Id = segment_id;

		// this is a subject of future re-evaluation - how to consider initial conditions for food-related patient state

		if (mParameters.D1_0 > 0) {
			Add_To_D1(mParameters.D1_0, mLast_Time, scgms::One_Minute * 15);
		}
		if (mParameters.Isc_0 > 0) {
			Add_To_Isc1(mParameters.Isc_0, mLast_Time, scgms::One_Minute * 15);
		}

		mInsulin_Pump.Initialize(mLast_Time, 0.0, 0.0, 0.0);

		for (auto& cmp : mCompartments) {
			cmp.Init(current_time);
		}

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

void CInfusion_Device::Set_Infusion_Parameter(double currentTime, double infusionRate, double infusionPeriod, double absorptionTime) {

	if (!std::isnan(infusionRate)) {

		if (mInfusion_Rate == 0.0) {
			mLast_Time = currentTime;
		}

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
