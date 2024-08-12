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

#include "v1_boluses.h"
#include "descriptor.h"

#include <scgms/rtl/SolverLib.h>

CV1_Boluses::~CV1_Boluses() {

}

void CV1_Boluses::Check_Bolus_Delivery(const double current_time) {
	if (!mBasal_Rate_Issued) {
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
		evt.signal_id() = scgms::signal_Requested_Insulin_Basal_Rate;
		evt.device_id() = icarus_v1_boluses::model_id;
		evt.device_time() = mStepped_Current_Time;
		evt.segment_id() = mSegment_id;
		evt.level() = mParameters.basal_rate;		
		mBasal_Rate_Issued = Succeeded(mOutput.Send(evt));
	}

	const double offset = current_time - mSimulation_Start;

	if (mBolus_Index < icarus_v1_boluses::meal_count) {
		if (offset >= mParameters.bolus[mBolus_Index].offset) {
			
			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
			evt.signal_id() = scgms::signal_Requested_Insulin_Bolus;
			evt.device_id() = icarus_v1_boluses::model_id;
			evt.device_time() = current_time;
			evt.segment_id() = mSegment_id;
			evt.level() = mParameters.bolus[mBolus_Index].bolus;

			if (Succeeded(mOutput.Send(evt))) {
				mBolus_Index++;
			}
		}
	}
}

HRESULT CV1_Boluses::Do_Execute(scgms::UDevice_Event event) {	
	return mOutput.Send(event);
}

HRESULT CV1_Boluses::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	return S_OK;
}

HRESULT IfaceCalling CV1_Boluses::Initialize(const double new_current_time, const uint64_t segment_id) {

	mStepped_Current_Time = mSimulation_Start = new_current_time;
	mBolus_Index = 0;
	mSegment_id = segment_id;
	mBasal_Rate_Issued = false;
	
	return S_OK;
}

HRESULT IfaceCalling CV1_Boluses::Step(const double time_advance_delta) {

	if (time_advance_delta > 0.0) {
		// advance the timestamp
		mStepped_Current_Time += time_advance_delta;
		Check_Bolus_Delivery(mStepped_Current_Time);
	}

	return S_OK;
}