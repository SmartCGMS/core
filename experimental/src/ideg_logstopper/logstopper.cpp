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

#include "logstopper.h"
#include "../descriptor.h"

#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/utils/math_utils.h"

CLog_Stopper_Filter::CLog_Stopper_Filter(scgms::IFilter* output) : CBase_Filter(output) {
	//
}


HRESULT IfaceCalling CLog_Stopper_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mStop_Time = configuration.Read_Double(ideg::logstopper::rsStop_Time);
	if (Is_Any_NaN(mStop_Time)) {
		error_description.push(L"Stop time cannot be NaN");
		return E_FAIL;
	}

	mState = NStop_State::Replaying;

	return S_OK;
}

HRESULT IfaceCalling CLog_Stopper_Filter::Do_Execute(scgms::UDevice_Event event) {

	switch (mState)
	{
		// forward everything until a specified time
		case NStop_State::Replaying:
			if (event.device_time() >= mStop_Time) {
				mState = NStop_State::Stopped;

				scgms::UDevice_Event evt(scgms::NDevice_Event_Code::Information);
				evt.info.set(L"ReplayStopped");
				evt.device_time() = event.device_time();
				evt.device_id() = ideg::logstopper::id;
				evt.segment_id() = scgms::All_Segments_Id;
				mOutput.Send(evt);

				// always pass shutdown
				if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
					break;

				return S_OK;
			}
			break;
		// do not forward events, wait for synchronization event
		case NStop_State::Stopped:
			if (event.signal_id() == scgms::signal_Synchronization) {
				mState = NStop_State::Resumed;
			}

			// always pass shutdown
			if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
				break;

			return S_OK;
		// forward everything until the end
		case NStop_State::Resumed:
			break;
	}

	return mOutput.Send(event);
}
