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

class CUVA_Padova_S2017_Discrete_Model;
struct CDiffusion_Compartments_State;

namespace uva_padova_S2017
{
	using TDiff_Eq_Fnc = double (CUVA_Padova_S2017_Discrete_Model::*)(const double, const double) const;
	using TDiff_Eq = decltype(std::bind<double>(std::declval<TDiff_Eq_Fnc>(), std::declval<CUVA_Padova_S2017_Discrete_Model*>(), std::declval<const decltype(std::placeholders::_1)&>(), std::declval<const decltype(std::placeholders::_2)&>()));

	// helper structure for equation binding
	struct CEquation_Binding
	{
		double& x;
		TDiff_Eq fnc;
	};

	struct CDiffusion_Compartment_Equation_Binding
	{
		CDiffusion_Compartments_State& x;
		TDiff_Eq fnc_input;
		TDiff_Eq fnc_intermediate;
		TDiff_Eq fnc_output;
	};
}

// helper structure for diffusion compartments 
struct CDiffusion_Compartments_State
{
	std::vector<double> quantity;
	double transferCoefficient = 0.0;

	double output() const {
		assert(quantity.size() > 0);
		return *quantity.rbegin();
	}

	size_t compartmentCount() const {
		return quantity.size();
	}

	void setCompartmentCount(size_t count) {

		// TODO: revisit this - shouldn't we preserve output quantity?

		if (count > compartmentCount())
		{
			quantity.reserve(count);
			while (compartmentCount() < count)
				quantity.push_back(0);
		}
		else
			quantity.resize(count);
	}

	size_t solverStateIdx = std::numeric_limits<size_t>::max(); // currently solved index
};

// state of UVa/Padova S2017 model equation system
struct CUVa_Padova_S2017_State
{
	double lastTime = 0.0;

	double Gp = std::numeric_limits<double>::quiet_NaN();
	double Gt = std::numeric_limits<double>::quiet_NaN();
	double Ip = std::numeric_limits<double>::quiet_NaN();
	double Il = std::numeric_limits<double>::quiet_NaN();
	double Qsto1 = std::numeric_limits<double>::quiet_NaN();
	double Qsto2 = std::numeric_limits<double>::quiet_NaN();
	double Qgut = std::numeric_limits<double>::quiet_NaN();
	double XL = std::numeric_limits<double>::quiet_NaN();
	double I = std::numeric_limits<double>::quiet_NaN();
	double XH = std::numeric_limits<double>::quiet_NaN();
	double X = std::numeric_limits<double>::quiet_NaN();
	double Isc1 = std::numeric_limits<double>::quiet_NaN();
	double Isc2 = std::numeric_limits<double>::quiet_NaN();
	double Iid1 = std::numeric_limits<double>::quiet_NaN();
	double Iid2 = std::numeric_limits<double>::quiet_NaN();
	double Iih = std::numeric_limits<double>::quiet_NaN();
	double Gsc = std::numeric_limits<double>::quiet_NaN();
	double H = std::numeric_limits<double>::quiet_NaN();
	double SRHS = std::numeric_limits<double>::quiet_NaN();
	double Hsc1 = std::numeric_limits<double>::quiet_NaN();
	double Hsc2 = std::numeric_limits<double>::quiet_NaN();

	CDiffusion_Compartments_State idt1;
	CDiffusion_Compartments_State idt2;
};

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

//DOI: 10.1177/1932296818757747
class CUVA_Padova_S2017_Discrete_Model : public scgms::CBase_Filter, public scgms::IDiscrete_Model
{
	private:
		// maximum accepted error estimate for ODE solvers for this model
		static constexpr double ODE_epsilon0 = 0.001;
		static constexpr size_t ODE_Max_Steps = 100;

	private:
		uva_padova_S2017::TParameters mParameters;

		std::mutex mStep_Mtx;

		// meal uptake accumulator
		Uptake_Accumulator mMeal_Ext;
		// bolus uptake accumulator
		Uptake_Accumulator mBolus_Insulin_Ext;
		// inhaled insulin uptake accumulator
		Uptake_Accumulator mInhaled_Insulin_Ext;
		// subcutaneous basal insulin uptake accumulator
		Uptake_Accumulator mSubcutaneous_Basal_Ext;
		// intradermal insulin uptake accumulator
		Uptake_Accumulator mIntradermal_Basal_Ext;
		// current state of Bergman model (all quantities)
		CUVa_Padova_S2017_State mState;
		// bound equations in a single vector - quantity and equation bound together
		const std::vector<uva_padova_S2017::CEquation_Binding> mEquation_Binding;
		// bound equations in a single vector - but instead of binding a single quantity, a diffusion compartment vector is bound instead
		const std::vector<uva_padova_S2017::CDiffusion_Compartment_Equation_Binding> mDiffusion_Compartment_Equation_Binding;

		struct TRequested_Amount
		{
			double time = 0;
			double amount = 0;
			bool requested = false;
		};

		TRequested_Amount mRequested_Subcutaneous_Insulin_Rate;
		TRequested_Amount mRequested_Intradermal_Insulin_Rate;
		std::vector<TRequested_Amount> mRequested_Insulin_Boluses;

		ode::default_solver ODE_Solver{ ODE_epsilon0, ODE_Max_Steps };

	private:
		// particular differential equations; they are bound to a specific CUVa_Padova_State (mState) field in mEquation_Binding upon model construction
		// they are never meant to change internal state - the model's Step method does it itself using ODE solver Step method result
		double eq_dGp(const double _T, const double _G) const;
		double eq_dGt(const double _T, const double _G) const;
		double eq_dIp(const double _T, const double _G) const;
		double eq_dIl(const double _T, const double _G) const;
		double eq_dQsto1(const double _T, const double _G) const;
		double eq_dQsto2(const double _T, const double _G) const;
		double eq_dQgut(const double _T, const double _G) const;
		double eq_dXL(const double _T, const double _G) const;
		double eq_dI(const double _T, const double _G) const;
		double eq_dXH(const double _T, const double _G) const;
		double eq_dX(const double _T, const double _G) const;
		double eq_dIsc1(const double _T, const double _G) const;
		double eq_dIsc2(const double _T, const double _G) const;
		double eq_dIid1(const double _T, const double _G) const;
		double eq_dIid2(const double _T, const double _G) const;
		double eq_dIih(const double _T, const double _G) const;
		double eq_dGsc(const double _T, const double _G) const;
		double eq_dH(const double _T, const double _G) const;
		double eq_dSRHS(const double _T, const double _G) const;
		double eq_dHsc1(const double _T, const double _G) const;
		double eq_dHsc2(const double _T, const double _G) const;

		// particular diffusion compartments differential equations
		double eq_didt1_input(const double _T, const double _G) const;
		double eq_didt1_intermediate(const double _T, const double _G) const;
		double eq_didt1_output(const double _T, const double _G) const;
		double eq_didt2_input(const double _T, const double _G) const;
		double eq_didt2_intermediate(const double _T, const double _G) const;
		double eq_didt2_output(const double _T, const double _G) const;

		// shared method to retrieve distribution parameter for gut transportion
		double Get_K_empt(const double _T) const;

	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		HRESULT Emit_Signal_Level(const GUID& signal_id, double device_time, double level);
		void Emit_All_Signals(double time_advance_delta);

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CUVA_Padova_S2017_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output);
		virtual ~CUVA_Padova_S2017_Discrete_Model() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
