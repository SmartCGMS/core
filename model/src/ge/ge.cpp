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

#include "ge.h"

#include <cmath>

CGE_Discrete_Model::CGE_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output) : CBase_Filter(output) {

}

CGE_Discrete_Model::~CGE_Discrete_Model() {

}

void CGE_Discrete_Model::Fixed_Step() {

}

void CGE_Discrete_Model::Emit_Current_State() {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = ge_model::model_id;
	evt.device_time() = mCurrent_Time;
	evt.level() = mOutput_Level;
	evt.signal_id() = ge_model::ge_signal_id;
	evt.segment_id() = mSegment_Id;

	Send(evt);
}


HRESULT CGE_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {

	if (event.event_code() == scgms::NDevice_Event_Code::Level) {
		if (event.signal_id() == scgms::signal_Carb_Intake) mWaiting_CHO += event.level();
		else if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate) {
			//TODO
		}
	}
	
	return Send(event);
}

HRESULT CGE_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	return E_NOTIMPL;
}


	
HRESULT IfaceCalling CGE_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id) {
	if (std::isnan(mCurrent_Time)) {
		mCurrent_Time = current_time;
		mSegment_Id = segment_id;
		return S_OK;
	}
	else
		return E_ILLEGAL_STATE_CHANGE;
}

HRESULT IfaceCalling CGE_Discrete_Model::Step(const double time_advance_delta) {
	
	HRESULT rc = E_INVALIDARG;

	if (time_advance_delta > 0.0) {

		mTime_Advance_Jitter += time_advance_delta;
		const bool executed_at_least_once = mTime_Advance_Jitter >= mTime_Fixed_Stepping;

		while (mTime_Advance_Jitter >= mTime_Fixed_Stepping) {
			mTime_Advance_Jitter -= mTime_Fixed_Stepping;
			mCurrent_Time += mTime_Fixed_Stepping;
			Fixed_Step();
			Emit_Current_State();
		}

		rc = executed_at_least_once ? S_OK : S_FALSE;
	} else if (time_advance_delta == 0.0) {
		Emit_Current_State();
		rc = S_OK;
	}

	return rc;
}

