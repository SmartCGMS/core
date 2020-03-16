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

#pragma once

#include <functional>

#include "../descriptor.h"
#include "../../../../common/rtl/FilterLib.h"

#include "../common/ode_solvers.h"
#include "../common/ode_solver_parameters.h"
#include "../common/uptake_accumulator.h"

// state of UVa/Padova model equation system
struct CUVa_Padova_State
{
	double lastTime;

	double Gp;
	double Gt;
	double Ip;
	double Il;
	double Qsto1;
	double Qsto2;
	double Qgut;
	double XL;
	double I;
	double X;
	double Isc1;
	double Isc2;
	double Gs;
	// not present in SimGlucose:
	double H;
	double XH;
	double SRHS;
	double Hsc1;
	double Hsc2;
};

class CUVa_Padova_Discrete_Model;

namespace uva_padova_model
{
	using TDiff_Eq_Fnc = double (CUVa_Padova_Discrete_Model::*)(const double, const double) const;
	using TDiff_Eq = decltype(std::bind<double>(std::declval<TDiff_Eq_Fnc>(), std::declval<CUVa_Padova_Discrete_Model*>(), std::declval<const decltype(std::placeholders::_1)&>(), std::declval<const decltype(std::placeholders::_2)&>()));

	// helper structure for equation binding
	struct CEquation_Binding
	{
		double& x;
		TDiff_Eq fnc;
	};
}

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CUVa_Padova_Discrete_Model : public virtual scgms::CBase_Filter, public virtual scgms::IDiscrete_Model
{
	private:
		// maximum accepted error estimate for ODE solvers for this model
		static constexpr double ODE_epsilon0 = 0.001;
		static constexpr size_t ODE_Max_Steps = 100;

	private:
		uva_padova_model::TParameters mParameters;

		// meal uptake accumulator
		Uptake_Accumulator mMeal_Ext;
		// bolus uptake accumulator
		Uptake_Accumulator mBolus_Ext;
		// basal uptake accumulator
		Uptake_Accumulator mBasal_Ext;
		// current state of Bergman model (all quantities)
		CUVa_Padova_State mState;
		// bound equations in a single vector - quantity and equation bound together
		const std::vector<uva_padova_model::CEquation_Binding> mEquation_Binding;

		struct TRequested_Amount
		{
			double time = 0;
			double amount = 0;
			bool requested = false;
		};

		TRequested_Amount mRequested_Basal;
		std::vector<TRequested_Amount> mRequested_Boluses;

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
		double eq_dX(const double _T, const double _G) const;
		double eq_dIsc1(const double _T, const double _G) const;
		double eq_dIsc2(const double _T, const double _G) const;
		double eq_dGs(const double _T, const double _G) const;
		// not present in SimGlucose:
		double eq_dH(const double _T, const double _G) const;
		double eq_dXH(const double _T, const double _G) const;
		double eq_dSRHS(const double _T, const double _G) const;
		double eq_dHsc1(const double _T, const double _G) const;
		double eq_dHsc2(const double _T, const double _G) const;

		// shared method to retrieve distribution parameter for gut transportion
		double Get_K_gut(const double _T) const;

	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		HRESULT Emit_Signal_Level(const GUID& signal_id, double device_time, double level);
		void Emit_All_Signals(double time_advance_delta);

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CUVa_Padova_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output);
		virtual ~CUVa_Padova_Discrete_Model() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
