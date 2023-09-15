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

#include "signal_feedback.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/string_utils.h"


class CAtomic_Counter {
protected:
	std::atomic<size_t>& mCounter;
public:
	CAtomic_Counter(std::atomic<size_t>& counter) : mCounter(counter) {
		counter++;
	}

	~CAtomic_Counter() {
		mCounter--;
	}


	size_t Value() {
		return mCounter.load();
	}
};

CSignal_Feedback::CSignal_Feedback(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CSignal_Feedback::~CSignal_Feedback() {
	//
}

HRESULT IfaceCalling CSignal_Feedback::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<scgms::IFilter_Feedback_Sender>(scgms::IID_Filter_Feedback_Sender, *riid, ppvObj)) return S_OK;
	return E_NOINTERFACE;
}

HRESULT CSignal_Feedback::Do_Execute(scgms::UDevice_Event event) {
	CAtomic_Counter stack_guard{ mStack_Counter };

	if (mStack_Counter >= mMaximum_Stack_Depth)
		return	EVENT_E_TOO_MANY_METHODS;	//looks better than ERROR_STACK_OVERFLOW, because we actually prevented the stack overflow

	auto send_to_receiver = [this](scgms::UDevice_Event &event)->HRESULT {
		if (!mReceiver)
			return ERROR_DS_DRA_EXTN_CONNECTION_FAILED;

		scgms::IDevice_Event* raw_event = event.get();
		event.release();
		return mReceiver->Execute(raw_event);
	};

	if (event.event_code() == scgms::NDevice_Event_Code::Level) {
		if (event.signal_id() == mSignal_ID) {
			if (mForward_Clone) {
				scgms::UDevice_Event clone = event.Clone();
				if (clone) {
					HRESULT rc = send_to_receiver(event);
					if (Succeeded(rc)) rc = mOutput.Send(clone);
					return rc;
				}
			}
			else
				return send_to_receiver(event);
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down) {
		mReceiver.reset();
	}

	return mOutput.Send(event);
}

HRESULT CSignal_Feedback::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	mFeedback_Name = configuration.Read_String(rsFeedback_Name);
	mSignal_ID = configuration.Read_GUID(rsSignal_Source_Id);
	mForward_Clone = !configuration.Read_Bool(rsRemove_From_Source);

	if (Is_Empty(mFeedback_Name) || Is_Invalid_GUID(mSignal_ID))
		return E_INVALIDARG;

	return S_OK;
}

HRESULT IfaceCalling CSignal_Feedback::Sink(scgms::IFilter_Feedback_Receiver *receiver) {

	if (receiver)
		mReceiver = refcnt::make_shared_reference_ext<scgms::SFilter_Feedback_Receiver, scgms::IFilter_Feedback_Receiver>(receiver, true);
	else
		mReceiver.reset();

	return S_OK;
}

HRESULT IfaceCalling CSignal_Feedback::Name(wchar_t** const name) {

	*name = const_cast<wchar_t*>(mFeedback_Name.c_str());
	return S_OK;
}
