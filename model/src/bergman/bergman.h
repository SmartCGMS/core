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

#include "../descriptor.h"
#include <scgms/rtl/FilterLib.h>

#include "../common/ode_solvers.h"
#include "../common/ode_solver_parameters.h"
#include "../common/uptake_accumulator.h"

// state of bergman minimal model equation system
struct CBergman_State
{
	double lastTime;

	double Q1;
	double Q2;
	double X;
	double I;
	double D1;
	double D2;
	double Isc;
	double Gsc;
};

class CBergman_Discrete_Model;

namespace bergman_model
{
	using TDiff_Eq_Fnc = double (CBergman_Discrete_Model::*)(const double, const double) const;
	using TDiff_Eq = decltype(std::bind<double>(std::declval<TDiff_Eq_Fnc>(), std::declval<CBergman_Discrete_Model*>(), std::declval<const decltype(std::placeholders::_1)&>(), std::declval<const decltype(std::placeholders::_2)&>()));

	// helper structure for equation binding
	struct CEquation_Binding
	{
		double& x;
		TDiff_Eq fnc;
	};
}

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CBergman_Discrete_Model : public scgms::CBase_Filter, public scgms::IDiscrete_Model
{
	private:
		// maximum accepted error estimate for ODE solvers for this model
		const double ODE_epsilon0 = 0.001;
		const size_t mODE_Max_Steps = 100;
	private:
		bergman_model::TParameters mParameters;

		// meal uptake accumulator
		Uptake_Accumulator mMeal_Ext;
		// bolus uptake accumulator
		Uptake_Accumulator mBolus_Ext;
		// basal uptake accumulator
		Uptake_Accumulator mBasal_Ext;
		// current state of Bergman model (all quantities)
		CBergman_State mState;
		bool mInitialized = false;
		// bound equations in a single vector - quantity and equation bound together
		const std::vector<bergman_model::CEquation_Binding> mEquation_Binding;

		double mLastBG, mLastIG;

		double mDiffIG_h_Time = -1;
		double mDiffIG_h_Value = 0.0;

		struct TRequested_Amount
		{
			double time = 0;
			double amount = 0;
			bool requested = false;
		};

		TRequested_Amount mRequested_Basal;
		std::vector<TRequested_Amount> mRequested_Boluses;

		ode::default_solver ODE_Solver{ ODE_epsilon0, mODE_Max_Steps };

	private:
		// particular differential equations; they are bound to a specific CBergman_State (mState) field in mEquation_Binding upon model construction
		// they are never meant to change internal state - the model's Step method does it itself using ODE solver Step method result
		double eq_dQ1(const double _T, const double _G) const;
		double eq_dQ2(const double _T, const double _G) const;
		double eq_dX(const double _T, const double _G) const;
		double eq_dI(const double _T, const double _G) const;
		double eq_dD1(const double _T, const double _G) const;
		double eq_dD2(const double _T, const double _G) const;
		double eq_dIsc(const double _T, const double _G) const;
		double eq_dGsc(const double _T, const double _G) const;

	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		HRESULT Emit_Signal_Level(const GUID& signal_id, double device_time, double level);
		void Emit_All_Signals(double time_advance_delta);

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CBergman_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output);
		virtual ~CBergman_Discrete_Model() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
