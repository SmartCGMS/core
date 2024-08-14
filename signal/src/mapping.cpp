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

#include "mapping.h"

#include <scgms/rtl/FilterLib.h>
#include <scgms/lang/dstrings.h>

CMapping_Filter::CMapping_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

HRESULT IfaceCalling CMapping_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {	
	mSource_Id = configuration.Read_GUID(rsSignal_Source_Id);
	mDestination_Id = configuration.Read_GUID(rsSignal_Destination_Id);

	if (Is_Invalid_GUID(mDestination_Id)) {     //mSource_Id can be Invalid_GUID due to external reasons, such as malformed CSV log events
		error_description.push(dsDestination_Signal_Cannot_Be_Invalid);
		return E_INVALIDARG;
	}
	mDestination_Null = mDestination_Id == scgms::signal_Null;

	return S_OK;
}

HRESULT IfaceCalling CMapping_Filter::Do_Execute(scgms::UDevice_Event event) {
	if (event.signal_id() == mSource_Id) {
		if (mDestination_Null && (event.event_code() != scgms::NDevice_Event_Code::Shut_Down)) { // && !event.is_control_event() && !event.is_info_event()) {
			event.reset(nullptr);
			return S_OK;
		}
		else {
			event.signal_id() = mDestination_Id;    //just changes the signal id
		}
	}

	return mOutput.Send(event);
}
