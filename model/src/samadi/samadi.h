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

#pragma once

#include <functional>
#include <mutex>

#include "../descriptor.h"
#include <scgms/rtl/FilterLib.h>

#include "../common/ode_solvers.h"
#include "../common/ode_solver_parameters.h"
#include "../common/uptake_accumulator.h"

class CSamadi_Discrete_Model;

namespace samadi_model {

	using TDiff_Eq_Fnc = double (CSamadi_Discrete_Model::*)(const double, const double) const;
	using TDiff_Eq = decltype(std::bind<double>(std::declval<TDiff_Eq_Fnc>(), std::declval<CSamadi_Discrete_Model*>(), std::declval<const decltype(std::placeholders::_1)&>(), std::declval<const decltype(std::placeholders::_2)&>()));

	// helper structure for equation binding
	struct CEquation_Binding {
		double& x;
		TDiff_Eq fnc;
	};
}

// state of Samadi model equation system
struct CSamadi_Model_State {
	double lastTime;

	double Q1;
	double Q2;
	double Gsub;
	double S1;
	double S2;
	double I;
	double x1;
	double x2;
	double x3;
	double D1;
	double D2;
	double DH1;
	double DH2;
	double E1;
	double E2;
	double TE;
};

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

//DOI: 10.1016/j.compchemeng.2019.106565
class CSamadi_Discrete_Model : public scgms::CBase_Filter, public scgms::IDiscrete_Model {
	private:
		// maximum accepted error estimate for ODE solvers for this model
		static constexpr double ODE_epsilon0 = 0.001;
		static constexpr size_t ODE_Max_Steps = 100;

	private:
		samadi_model::TParameters mParameters;

		std::mutex mStep_Mtx;

		// meal uptake accumulator
		Uptake_Accumulator mMeal_Ext;
		// rescue meal uptake accumulator
		Uptake_Accumulator mRescue_Meal_Ext;
		// bolus uptake accumulator
		Uptake_Accumulator mBolus_Insulin_Ext;
		// subcutaneous basal insulin uptake accumulator
		Uptake_Accumulator mSubcutaneous_Basal_Ext;
		// heart rate storage (although not "uptake"
		Uptake_Accumulator mHeart_Rate;
		// current state of Bergman model (all quantities)
		CSamadi_Model_State mState;
		bool mInitialized = false;
		// bound equations in a single vector - quantity and equation bound together
		const std::vector<samadi_model::CEquation_Binding> mEquation_Binding;

		struct TRequested_Amount {
			double time = 0;
			double amount = 0;
			bool requested = false;
		};

		TRequested_Amount mRequested_Subcutaneous_Insulin_Rate;
		TRequested_Amount mRequested_Intradermal_Insulin_Rate;
		std::vector<TRequested_Amount> mRequested_Insulin_Boluses;

		// RK-based solvers are very precise, however much more computationally intensive
		//ode::default_solver ODE_Solver{ ODE_epsilon0, ODE_Max_Steps };
		// Let us use euler solver to speed things up during debug phase
		ode::euler::CSolver ODE_Solver;

	private:
		// particular differential equations; they are bound to a specific CSamadi_Model_State (mState) field in mEquation_Binding upon model construction
		// they are never meant to change internal state - the model's Step method does it itself using ODE solver Step method result
		double eq_dQ1(const double _T, const double _G) const;
		double eq_dQ2(const double _T, const double _G) const;
		double eq_dGsub(const double _T, const double _G) const;
		double eq_dS1(const double _T, const double _G) const;
		double eq_dS2(const double _T, const double _G) const;
		double eq_dI(const double _T, const double _G) const;
		double eq_dx1(const double _T, const double _G) const;
		double eq_dx2(const double _T, const double _G) const;
		double eq_dx3(const double _T, const double _G) const;
		double eq_dD1(const double _T, const double _G) const;
		double eq_dD2(const double _T, const double _G) const;
		double eq_dDH1(const double _T, const double _G) const;
		double eq_dDH2(const double _T, const double _G) const;
		double eq_dE1(const double _T, const double _G) const;
		double eq_dE2(const double _T, const double _G) const;
		double eq_dTE(const double _T, const double _G) const;

	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		HRESULT Emit_Signal_Level(const GUID& signal_id, double device_time, double level);
		void Emit_All_Signals(double time_advance_delta);

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CSamadi_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output);
		virtual ~CSamadi_Discrete_Model() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
