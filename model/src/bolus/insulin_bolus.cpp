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

#include "insulin_bolus.h"
#include <scgms/rtl/SolverLib.h>

CDiscrete_Insulin_Bolus_Calculator::CDiscrete_Insulin_Bolus_Calculator(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<insulin_bolus::TParameters>(parameters, insulin_bolus::default_parameters)) {
	//
}

HRESULT CDiscrete_Insulin_Bolus_Calculator::Do_Execute(scgms::UDevice_Event event) {
	if (event.event_code() == scgms::NDevice_Event_Code::Level) {
		if (event.signal_id() == scgms::signal_Carb_Intake) {
			const double bolus_level = event.level() * mParameters.csr; //compute while we still own the event
			const double bolus_time = event.device_time();
			const uint64_t bolus_segment = event.segment_id();
			HRESULT rc = mOutput.Send(event);
			if (Succeeded(rc)) {
				scgms::UDevice_Event bolus{ scgms::NDevice_Event_Code::Level };
				bolus.level() = bolus_level;
				bolus.device_time() = bolus_time;
				bolus.segment_id() = bolus_segment;
				bolus.signal_id() = scgms::signal_Requested_Insulin_Bolus;
				rc = mOutput.Send(bolus);
			}

			return rc;
		}
	}

	return mOutput.Send(event);
}

HRESULT CDiscrete_Insulin_Bolus_Calculator::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// configured in the constructor
	return S_OK;
}

HRESULT IfaceCalling CDiscrete_Insulin_Bolus_Calculator::Initialize(const double current_time, const uint64_t segment_id) {
    return S_OK;    //time and segment independet for now
}

HRESULT IfaceCalling CDiscrete_Insulin_Bolus_Calculator::Step(const double time_advance_delta) {
	return S_OK;    //time independet for now
}
