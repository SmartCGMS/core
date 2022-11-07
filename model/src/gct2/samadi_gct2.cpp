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

#include "samadi_gct2.h"

#include "../../../../common/rtl/SolverLib.h"

// this selects GCTv2 support implementations
using namespace gct2_model;
using namespace samadi_gct2_model;

/** model-wide constants **/

constexpr const double Glucose_Molar_Weight = 180.156; // [g/mol]

constexpr bool Improved_Version = true;

/*
static inline void Ensure_Min_Value(double& target, double value) {
	target = std::max(target, value);
}
*/

/**
 * Samadi GCTv2 model implementation
 */

CSamadi_GCT2_Discrete_Model::CSamadi_GCT2_Discrete_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output) :
	CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<samadi_model::TParameters>(parameters, samadi_model::default_parameters.vector)),

	mHeart_Rate(mCompartments[NGCT_Compartment::Heart_Rate].Create_Depot<CExternal_State_Depot>(mParameters.HRbase, false)),
	mSink(mCompartments[NGCT_Compartment::Generic_Sink].Create_Depot<CSink_Depot>(1.0, false)),
	mGlucose_Source(mCompartments[NGCT_Compartment::Generic_Source].Create_Depot<CSource_Depot>(1.0, false))
{
	mHeart_Rate.Set_Solution_Volume(1.0); // so the concentration numerically equals mass

	mHeart_Rate.Set_Persistent(true).Set_Name(L"HR");
	mSink.Set_Persistent(true).Set_Name(L"Sink");
	mGlucose_Source.Set_Persistent(true).Set_Name(L"Q_source");

	// base depots
	auto& q1 =  mCompartments[NGCT_Compartment::Q1].Create_Depot(mParameters.Q1_0, false).Set_Persistent(true).Set_Solution_Volume(mParameters.Vg * mParameters.BW).Set_Name(L"Q1");
	auto& q2 =  mCompartments[NGCT_Compartment::Q2].Create_Depot(mParameters.Q2_0, false).Set_Persistent(true).Set_Solution_Volume(mParameters.Vg * mParameters.BW).Set_Name(L"Q2");
	auto& qsc = mCompartments[NGCT_Compartment::Gsub].Create_Depot(mParameters.Gsub_0 * (mParameters.Vg * mParameters.BW), false).Set_Persistent(true).Set_Solution_Volume(mParameters.Vg * mParameters.BW).Set_Name(L"Qsc");
	auto& s2 = mCompartments[NGCT_Compartment::S2].Create_Depot(mParameters.S2_0, false).Set_Persistent(true).Set_Solution_Volume(mParameters.Vi * mParameters.BW).Set_Name(L"S2");
	auto& i = mCompartments[NGCT_Compartment::I].Create_Depot(mParameters.I_0, false).Set_Persistent(true).Set_Solution_Volume(mParameters.Vi * mParameters.BW).Set_Name(L"I");

	auto& x1 = mCompartments[NGCT_Compartment::x1].Create_Depot(mParameters.x1_0, false).Set_Persistent(true).Set_Name(L"x1");
	auto& x2 = mCompartments[NGCT_Compartment::x2].Create_Depot(mParameters.x2_0, false).Set_Persistent(true).Set_Name(L"x2");
	auto& x3 = mCompartments[NGCT_Compartment::x3].Create_Depot(mParameters.x3_0, false).Set_Persistent(true).Set_Name(L"x3");

	auto& D2 = mCompartments[NGCT_Compartment::D2].Create_Depot(0.0/*mParameters.D2_0*/, false).Set_Persistent(true).Set_Name(L"D2");
	auto& DH2 = mCompartments[NGCT_Compartment::DH2].Create_Depot(0.0/*mParameters.DH2_0*/, false).Set_Persistent(true).Set_Name(L"DH2");

	auto& E1 = mCompartments[NGCT_Compartment::E1].Create_Depot(0.0/*mParameters.E1_0*/, false);
	E1.Set_Persistent(true).Set_Name(L"E1");
	auto& E2 = mCompartments[NGCT_Compartment::E2].Create_Depot(0.0/*mParameters.E2_0*/, false);
	E2.Set_Persistent(true).Set_Name(L"E2");
	auto& TE = mCompartments[NGCT_Compartment::TE].Create_Depot(0.0/*mParameters.TE_0*/, false);
	TE.Set_Persistent(true).Set_Name(L"TE");

	//// Insulin subsystem links

	// S1-S2 is linked on-demand when adding a new depot

	// S2-I
	s2.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(i,
		[this](CDepot_Link& link) {
			link.Set_Unit_Converter(1.0 / (mParameters.Vi * mParameters.BW));
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0 / mParameters.tmaxi
	);

	// I-x1
	mGlucose_Source.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(x1,
		[&i](CDepot_Link& link) {
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(i, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.kb1
	);

	// I-x2
	mGlucose_Source.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(x2,
		[&i](CDepot_Link& link) {
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(i, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.kb2
	);

	// I-x3
	mGlucose_Source.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(x3,
		[&i](CDepot_Link& link) {
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(i, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.kb3
	);

	// I-sink
	i.Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ke
	);

	// x1-sink
	x1.Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ka1
	);

	// x2-sink
	x2.Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ka2
	);

	// x3-sink
	x3.Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.ka3
	);

	//// Carbs subsystem links

	// D1-D2 and DH1-DH2 are linked on-demand when adding a new depot

	// D2-Q1
	D2.Link_To<CConstant_Unbounded_Transfer_Function>(q1,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0 / mParameters.tmaxG
	);

	// DH2-Q1
	DH2.Link_To<CConstant_Unbounded_Transfer_Function>(q1,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		2.0 / mParameters.tmaxG
	);

	//// Exercise subsystem links

	// HR-E1

	/*mGlucose_Source.Link_To<CConstant_Unbounded_Transfer_Function>(E1,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.HRbase / mParameters.t_HR
	).Set_Source_Quantity_Dependent(false);

	mGlucose_Source.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(E1,
		[this](CDepot_Link& link) {
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(mHeart_Rate, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0 / mParameters.t_HR
	).Set_Source_Quantity_Dependent(false);*/

	// E1-sink
	/*E1.Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0 / mParameters.t_HR
	);

	// source-TE
	mGlucose_Source.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(TE,
		[&E1, this](CDepot_Link& link) {
			link.Add_Moderator<CSamadi_PA_Moderation_No_Elimination_Function>(E1, mParameters.alpha, mParameters.HRbase, mParameters.n);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.c1 / mParameters.t_ex
	).Set_Source_Quantity_Dependent(false);

	// source-TE
	mGlucose_Source.Link_To<CConstant_Unbounded_Transfer_Function>(TE, 
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.c2 / mParameters.t_ex
	).Set_Source_Quantity_Dependent(false);

	// TE-sink
	TE.Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0 / mParameters.t_ex
	);

	// E2-sink
	E2.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		[&E1, this](CDepot_Link& link) {
			link.Add_Moderator<CSamadi_PA_Moderation_No_Elimination_Function>(E1, mParameters.alpha, mParameters.HRbase, mParameters.n);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0 / mParameters.t_in
	);

	// E2-sink
	E2.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		[&TE, this](CDepot_Link& link) {
			link.Add_Moderator<CInverse_Linear_Moderation_No_Elimination_Function>(TE, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0
	);

	// source-E2
	mGlucose_Source.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(E2,
		[&E1, &TE, this](CDepot_Link& link) {
			link.Add_Moderator<CSamadi_PA_Moderation_No_Elimination_Function>(E1, mParameters.alpha, mParameters.HRbase, mParameters.n);
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(TE, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0 / (mParameters.c1 + mParameters.c2)
	).Set_Source_Quantity_Dependent(false);*/

	//// Glucose subsystem links

	// Q1-Q2
	q1.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(q2,
		[&x1](CDepot_Link& link) {
			// elimination with 1+alpha*E1^2 rate
			//link.Add_Moderator<CQuadratic_Moderation_No_Elimination_Function>(E2, mParameters.alpha);
			// also moderated by the presence of insulin
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(x1, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0
	);

	// Q2-Q1
	q2.Link_To<CConstant_Unbounded_Transfer_Function>(q1,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.k12
	);

	// F01C(t) term
	q1.Link_To<CConcentration_Threshold_Ratio_Disappearance_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(q1),
		4.5, // 4.5 mmol/L threshold, as given by the article
		mParameters.F01 * mParameters.BW,
		true
	).Set_Source_Quantity_Dependent(false);

	// F_R(t) term (glycosuria)
	q1.Link_To<CConcentration_Threshold_Disappearance_Unbounded_Transfer_Function>(mSink,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		std::ref(q1),
		9.0, // 9.0 mmol/L threshold, as given by the article
		0.003 * mParameters.Vg * mParameters.BW
	).Set_Source_Quantity_Dependent(false);

	// Q1-sink
	q1.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		[&x3](CDepot_Link& link) {
			// also moderated by the presence of insulin
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(x3, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.EGP_0 * mParameters.BW
	).Set_Source_Quantity_Dependent(false);

	// source-Q1
	mGlucose_Source.Link_To<CConstant_Unbounded_Transfer_Function>(q1,
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.EGP_0 * mParameters.BW
	).Set_Source_Quantity_Dependent(false);

	// Q2-sink
	q2.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		[&x2](CDepot_Link& link) {
			// elimination with 1+alpha*E1^2 rate
			//link.Add_Moderator<CQuadratic_Moderation_No_Elimination_Function>(E2, mParameters.alpha);
			// also moderated by the presence of insulin
			link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(x2, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		1.0
	);

	// Q2-sink
	q2.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
		[](CDepot_Link& link) {
			// elimination based on exercise
			//link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(E1, 1.0);
		},
		CTransfer_Function::Start,
		CTransfer_Function::Unlimited,
		mParameters.beta / mParameters.HRbase
	).Set_Source_Quantity_Dependent(false);


	if constexpr (Improved_Version)
	{
		q1.Link_To<CTwo_Way_Diffusion_Unbounded_Transfer_Function>(qsc,
			CTransfer_Function::Start,
			CTransfer_Function::Unlimited,
			std::ref(q1), std::ref(qsc),
			1.0 / mParameters.tau_g
		);
	}
	else
	{
		// Q1-Qsc
		mGlucose_Source.Moderated_Link_To<CConstant_Unbounded_Transfer_Function>(qsc,
			[&q1](CDepot_Link& link) {
				// elimination based on exercise
				link.Add_Moderator<CLinear_Moderation_No_Elimination_Function>(q1, 1.0);
			},
			CTransfer_Function::Start,
			CTransfer_Function::Unlimited,
			1.0 / mParameters.tau_g
		);

		// Qsc-sink
		qsc.Link_To<CConstant_Unbounded_Transfer_Function>(mSink,
			CTransfer_Function::Start,
			CTransfer_Function::Unlimited,
			1.0 / mParameters.tau_g
		);
	}
}

CDepot& CSamadi_GCT2_Discrete_Model::Add_Carbs(double amount, double start, double duration, bool rescue) {

	CDepot& depot = mCompartments[rescue ? NGCT_Compartment::DH1 : NGCT_Compartment::D1].Create_Depot(0.78 /* compensating for buggy original Samadi */ * mParameters.Ag * amount / Glucose_Molar_Weight, false);

	depot.Set_Name(std::wstring((rescue ? L"DH1" : L"D1" ) + std::to_wstring(amount) + L")"));

	depot.Link_To<CConstant_Unbounded_Transfer_Function>(mCompartments[rescue ? NGCT_Compartment::DH2 : NGCT_Compartment::D2].Get_Persistent_Depot(), start / scgms::One_Minute, 10* duration / scgms::One_Minute,
		rescue ?
			2.0/(mParameters.tmaxG)
			:
			1.0/(mParameters.tmaxG)
		);

	return depot;
}

CDepot& CSamadi_GCT2_Discrete_Model::Add_Insulin(double amount, double start, double duration) {

	CDepot& depot = mCompartments[NGCT_Compartment::S1].Create_Depot(amount, false);

	depot.Set_Name(std::wstring(L"Isc1 (") + std::to_wstring(amount) + L")");

	depot.Link_To<CConstant_Unbounded_Transfer_Function>(mCompartments[NGCT_Compartment::S2].Get_Persistent_Depot(), start / scgms::One_Minute, 10* duration / scgms::One_Minute, 1.0 / mParameters.tmaxi);

	return depot;
}

void CSamadi_GCT2_Discrete_Model::Emit_All_Signals(double time_advance_delta) {

	const double _T = mLast_Time;

	/*
	 * Pending signals
	 */

	for (const auto& pending : mPending_Signals) {
		Emit_Signal_Level(pending.signal_id, pending.time, pending.value);
	}

	mPending_Signals.clear();

	const double iob = (mCompartments[NGCT_Compartment::S1].Get_Quantity() + mCompartments[NGCT_Compartment::S2].Get_Quantity()) / 1000.0;
	Emit_Signal_Level(samadi_gct2_model::signal_IOB, _T, iob);

	const double cob = (mCompartments[NGCT_Compartment::D1].Get_Quantity() + mCompartments[NGCT_Compartment::D2].Get_Quantity() + mCompartments[NGCT_Compartment::DH1].Get_Quantity() + mCompartments[NGCT_Compartment::DH2].Get_Quantity()) * Glucose_Molar_Weight / 1000.0;
	Emit_Signal_Level(samadi_gct2_model::signal_COB, _T, cob);

	/*
	 * Virtual sensor signals
	 */

	// BG
	const double bglevel = mCompartments[NGCT_Compartment::Q1].Get_Concentration();
	Emit_Signal_Level(samadi_gct2_model::signal_BG, _T, bglevel);

	// IG - CGM
	const double iglevel = mCompartments[NGCT_Compartment::Gsub].Get_Concentration();
	Emit_Signal_Level(samadi_gct2_model::signal_IG, _T, iglevel);
}

HRESULT CSamadi_GCT2_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {
	HRESULT res = S_FALSE;

	if (!std::isnan(mLast_Time)) {

		if (event.event_code() == scgms::NDevice_Event_Code::Level) {

			// insulin pump control
			if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate) {

				if (event.device_time() >= mLast_Time)
				{
					constexpr double absorptionTime = 3.0_hr;
					constexpr double PortionTimeSpacing = 60_sec;
					const double rate = event.level() / (60.0 * (60_sec / PortionTimeSpacing));

					mInsulin_Pump.Set_Infusion_Parameter(event.device_time(), rate, PortionTimeSpacing, absorptionTime);
					mPending_Signals.push_back(TPending_Signal{ scgms::signal_Delivered_Insulin_Basal_Rate, event.device_time(), event.level() });

					res = S_OK;
				}
			}
			// physical activity
			else if (event.signal_id() == scgms::signal_Heartbeat) {

				if (event.device_time() >= mLast_Time)
					mHeart_Rate.Set_Quantity(event.level());
			}
			// bolus insulin
			else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus || event.signal_id() == scgms::signal_Delivered_Insulin_Bolus) {

				if (event.device_time() >= mLast_Time)
				{
					constexpr double absorptionTime = 3.0_hr;

					Add_Insulin(event.level()*1000*1.15 /* to compensate for buggy original Samadi; don't blame me, I hate the model */, event.device_time() + scgms::One_Minute, absorptionTime);

					mPending_Signals.push_back(TPending_Signal{ scgms::signal_Delivered_Insulin_Bolus, event.device_time(), event.level() });

					res = S_OK;
				}
			}
			// carbs intake; NOTE: this should be further enhanced once architecture supports meal parametrization (e.g.; glycemic index, ...)
			else if ((event.signal_id() == scgms::signal_Carb_Intake) || (event.signal_id() == scgms::signal_Carb_Rescue)) {

				constexpr double MinsEating = 10.0;
				constexpr double InvMinsEating = 1.0 / MinsEating;

				const double PortionSize = 1000.0 * event.level() * InvMinsEating;

				{
					double aoff = 0;
					do {						
						Add_Carbs(PortionSize, event.device_time() + aoff * scgms::One_Minute, 2.0_hr, (event.signal_id() == scgms::signal_Carb_Rescue));
						aoff += 1.0;
					} while (aoff < MinsEating);
				}

				// res = S_OK; - do not unless we have another signal called consumed CHO
			}
		}
	}

	if (res == S_FALSE)
		res = mOutput.Send(event);

	return res;
}

HRESULT CSamadi_GCT2_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor - no need for additional configuration; signalize success
	return S_OK;
}

HRESULT IfaceCalling CSamadi_GCT2_Discrete_Model::Step(const double time_advance_delta) {

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
				if (mInsulin_Pump.Get_Dosage(mLast_Time, dosage))
					Add_Insulin(dosage.amount * 1000.0, dosage.start, dosage.duration);

				// step all compartments
				std::for_each(std::execution::seq, mCompartments.begin(), mCompartments.end(), [this](CCompartment& comp) {
					comp.Step(mLast_Time / scgms::One_Minute);
				});

				// commit all compartments
				std::for_each(std::execution::seq, mCompartments.begin(), mCompartments.end(), [this](CCompartment& comp) {
					comp.Commit(mLast_Time / scgms::One_Minute);
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

HRESULT CSamadi_GCT2_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = samadi_model::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;

	return mOutput.Send(evt);
}

HRESULT IfaceCalling CSamadi_GCT2_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {

	if (std::isnan(mLast_Time)) {

		mLast_Time = current_time;
		mSegment_Id = segment_id;

		// this is a subject of future re-evaluation - how to consider initial conditions for food-related patient state

		//if (mParameters.D1_0 > 0)
		//	Add_Carbs(mParameters.D1_0, mLast_Time, 60.0_min, false);
		//if (mParameters.DH1_0 > 0)
		//	Add_Carbs(mParameters.DH1_0, mLast_Time, 60.0_min, true);
		//if (mParameters.Isc_0 > 0)
		//	Add_Insulin(mParameters.I, mLast_Time, scgms::One_Minute * 15);

		mInsulin_Pump.Initialize(mLast_Time, 0.0, 0.0, 0.0);

		for (auto& cmp : mCompartments)
			cmp.Init(current_time / scgms::One_Minute);

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

		if (mInfusion_Rate == 0.0)
			mLast_Time = currentTime;

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
