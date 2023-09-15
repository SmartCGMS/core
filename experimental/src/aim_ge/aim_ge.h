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

#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/referencedImpl.h"
#include "../../../../common/rtl/UILib.h"

#include <cmath>

#include "../descriptor.h"
#include "aim_ge_vm.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CAIM_GE_Model : public virtual scgms::CBase_Filter, public virtual scgms::IDiscrete_Model, public aim_ge::IQuantity_Source
{
	private:
		aim_ge::TParameters mParameters;

		uint64_t mSegment_id = scgms::Invalid_Segment_Id;

		// we have to hold the current timestamp
		double mCurrent_Time = 0;

		aim_ge::CContext mContext;

		std::vector<double> mPastIG;

		struct Stored_Signal {
			double time_start, time_end;
			double value;
		};

		std::list<Stored_Signal> mStored_Insulin;
		std::list<Stored_Signal> mStored_Carbs;

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

		void Advance_Model(double time_advance_delta);
		bool Emit_IG(double level, double time);

		double Calc_IOB_At(const double time);
		double Calc_COB_At(const double time);

	public:
		CAIM_GE_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output);
		virtual ~CAIM_GE_Model() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double new_current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;

		// aim_ge::IQuantity_Source iface
		virtual double Get_Input_Quantity(aim_ge::NInput_Quantity q, size_t timeOffset) override;
};

#pragma warning( pop )

