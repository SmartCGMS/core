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

#include "../descriptor.h"
#include <scgms/rtl/FilterLib.h>

#include "gct2_transfer_functions.h"
#include "gct2_moderation_functions.h"
#include "gct2_depot.h"

namespace samadi_gct2_model {

	enum class NGCT_Compartment : size_t {
		// physical compartments
		Q1,
		Q2,
		Gsub,
		S1,
		S2,
		I,
		x1,
		x2,
		x3,
		D1,
		D2,
		DH1,
		DH2,
		E1,
		TE,
		E2,

		Heart_Rate,
		Generic_Sink,
		Generic_Source,

		count
	};

	constexpr size_t GCT_Compartment_Count = static_cast<size_t>(NGCT_Compartment::count);

	/**
	 * Class representing a vector of compartments within GCT model
	 */
	class CGCT_Compartments final : public std::vector<gct2_model::CCompartment> {
		public:
			CGCT_Compartments() : std::vector<gct2_model::CCompartment>(GCT_Compartment_Count) {
				//
			}

			gct2_model::CCompartment& operator[](NGCT_Compartment idx) {
				return std::vector<gct2_model::CCompartment>::operator[](static_cast<size_t>(idx));
			}

			const gct2_model::CCompartment& operator[](NGCT_Compartment idx) const {
				return std::vector<gct2_model::CCompartment>::operator[](static_cast<size_t>(idx));
			}
	};

	/**
	 * Container of dosage parameters
	 */
	struct TDosage {
		double amount;
		double start;
		double duration;
	};

	/**
	 * Class representing a device connected to the patient
	 * TODO: this should be further decomposed into a separate module
	 */
	class CInfusion_Device {

		protected:
			double mLast_Time = 0.0;
			double mInfusion_Rate = 0.0;
			double mInfusion_Period = 0.0;
			double mAbsorption_Time = 0.0;

		public:
			// initializes device with current time and primary infusion rate with infusion period
			void Initialize(double initialTime, double infusionRate, double infusionPeriod, double absorptionTime);

			// sets infusion rate and infusion rate; set to quiet NaN if no change is intended to be made
			void Set_Infusion_Parameter(double currentTime, double infusionRate, double infusionPeriod = std::numeric_limits<double>::quiet_NaN(), double absorptionTime = std::numeric_limits<double>::quiet_NaN());

			// retrieves dosage at given time; returns true if 'dosage' was filled with valid values to be infused,
			// false otherwise; this function should be called repeatedly until it returns false
			bool Get_Dosage(double currentTime, TDosage& dosage);
	};

	/**
	 * Pending signal (dosed as input but not yet emitted as output signal)
	 */
	struct TPending_Signal {
		GUID signal_id;
		double time;
		double value;
	};

}

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CSamadi_GCT2_Discrete_Model : public scgms::CBase_Filter, public scgms::IDiscrete_Model {

	private:
		// last step time; NaN until initialized by outer code
		double mLast_Time = std::numeric_limits<double>::quiet_NaN();
		// model parameters
		samadi_model::TParameters mParameters;

		// all compartments present in system
		samadi_gct2_model::CGCT_Compartments mCompartments;

		// signals transformed to output signals, not yet emitted
		std::list<samadi_gct2_model::TPending_Signal> mPending_Signals;

		// pump device (doses insulin at set basal rate)
		samadi_gct2_model::CInfusion_Device mInsulin_Pump;

		// physical activity external depot reference
		gct2_model::CExternal_State_Depot& mHeart_Rate;
		// insulin sink report to properly link bolus/basal injections to it to simulate local degradation
		gct2_model::CDepot& mSink;
		// glucose source (unlimited)
		gct2_model::CDepot& mGlucose_Source;

	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		HRESULT Emit_Signal_Level(const GUID& signal_id, double device_time, double level);
		void Emit_All_Signals(double time_advance_delta);

		// adds depot to D1 compartment and links to a new compartment in D2
		gct2_model::CDepot& Add_Carbs(double amount, double start, double duration, bool rescue);
		// adds depot to Isc_1 compartment and links to a new compartment in Isc_2
		gct2_model::CDepot& Add_Insulin(double amount, double start, double duration);

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CSamadi_GCT2_Discrete_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output);
		virtual ~CSamadi_GCT2_Discrete_Model() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
