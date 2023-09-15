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

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/UILib.h"

#include "descriptor.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance


class CV1_Boluses : public virtual scgms::CDiscrete_Model<icarus_v1_boluses::TParameters> {
protected:
	double mSimulation_Start = std::numeric_limits<double>::quiet_NaN();
	double mStepped_Current_Time = std::numeric_limits<double>::quiet_NaN();
	size_t mBolus_Index = std::numeric_limits<size_t>::max();
	uint64_t mSegment_id = scgms::Invalid_Segment_Id;
	bool mBasal_Rate_Issued = false;

		//delivers insulin bolus, only if it is time is LESS-ONLY than the current_time
		//LESS-ONLY let us keep the code short and clean
	void Check_Bolus_Delivery(const double current_time);
protected:
	// scgms::CBase_Filter iface implementation
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CV1_Boluses(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output) :
		CBase_Filter(output, icarus_v1_boluses::model_id),
		scgms::CDiscrete_Model<icarus_v1_boluses::TParameters>(parameters, icarus_v1_boluses::default_parameters, output, icarus_v1_boluses::model_id)
		{}
	virtual ~CV1_Boluses();

	// scgms::IDiscrete_Model iface
	virtual HRESULT IfaceCalling Initialize(const double new_current_time, const uint64_t segment_id) override final;
	virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
