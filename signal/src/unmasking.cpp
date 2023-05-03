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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "unmasking.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/string_utils.h"

#include <algorithm>
#include <cctype>

CUnmasking_Filter::CUnmasking_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

HRESULT IfaceCalling CUnmasking_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mSignal_Id = configuration.Read_GUID(rsSignal_Masked_Id);
	if (Is_Invalid_GUID(mSignal_Id)) return E_INVALIDARG;

	return S_OK;
}

HRESULT IfaceCalling CUnmasking_Filter::Do_Execute(scgms::UDevice_Event event) {
	// mask only configured signal and event of type "Level"
	if (event.event_code() == scgms::NDevice_Event_Code::Masked_Level && event.signal_id() == mSignal_Id) {


		scgms::TDevice_Event* raw;
		if (event.get()->Raw(&raw) == S_OK)
			raw->event_code = scgms::NDevice_Event_Code::Level;
	}


	return mOutput.Send(event);
}