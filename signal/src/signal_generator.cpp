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
 * Univerzitni 8
 * 301 00, Pilsen
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

#pragma once

#include "signal_generator.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

CSignal_Generator::CSignal_Generator(glucose::IFilter *output) : CBase_Filter(output) {
	//
}

CSignal_Generator::~CSignal_Generator() {
	Stop_Generator();
}


void CSignal_Generator::Stop_Generator() {
	mQuitting = true;
	if (mThread) {
		if (mThread->joinable()) 
			mThread->join();			
		mThread.reset();
	}
}

HRESULT CSignal_Generator::Do_Execute(glucose::UDevice_Event event) {

	HRESULT rc = E_UNEXPECTED;

	if ((mSync_To_Signal) && ((mTotal_Time < mMax_Time) || (mMax_Time <= 0.0))) {

		if (event.event_code() == glucose::NDevice_Event_Code::Time_Segment_Start) 
			mLast_Device_Time = std::numeric_limits<double>::quiet_NaN();

		const bool step_the_model = event.signal_id() == mSync_Signal;
		double dynamic_stepping = 0.0;			//means "emit current state"
		if (step_the_model) {
			if (!isnan(mLast_Device_Time)) dynamic_stepping = event.device_time() - mLast_Device_Time;
				else {
																	//cannot advance the model because this is the very first event, thus we do not have the delta
					mModel->Set_Current_Time(event.device_time());	//for which we need to set the current time
				}

			mLast_Device_Time = event.device_time();
		}

		glucose::IDevice_Event *raw_event = event.get();
		event.release();
		HRESULT rc = mModel->Execute(raw_event);
		
		if (step_the_model) rc = mModel->Step(dynamic_stepping);
	}
	else {

		bool shutdown = (event.event_code() == glucose::NDevice_Event_Code::Shut_Down);

		glucose::IDevice_Event *raw_event = event.get();
		event.release();
		rc = mModel->Execute(raw_event);

		if (shutdown) {
			Stop_Generator();
			mModel.reset();
		}
	}

	return rc;
}

HRESULT CSignal_Generator::Do_Configure(glucose::SFilter_Configuration configuration) {
	Stop_Generator();

	mSync_To_Signal = configuration.Read_Bool(rsSynchronize_to_Signal);
	mSync_Signal = configuration.Read_GUID(rsSynchronization_Signal);
	mFeedback_Name = configuration.Read_String(rsFeedback_Name);
	mFixed_Stepping = configuration.Read_Double(rsStepping);
	mMax_Time = configuration.Read_Double(rsMaximum_Time);
	mEmit_Shutdown = configuration.Read_Bool(rsShutdown_After_Last);

	std::vector<double> lower, parameters, upper;
	configuration.Read_Parameters(rsParameters, lower, parameters, upper);

	const GUID model_id = configuration.Read_GUID(rsSelected_Model);
	mModel = glucose::SDiscrete_Model { model_id, parameters, mOutput };
	if (!mModel)
		return E_FAIL;

	HRESULT rc = mModel->Configure(configuration.get());
	if (!SUCCEEDED(rc))
		return rc;

	mTotal_Time = 0.0;
	mQuitting = false;

	if (!mSync_To_Signal) {
		mThread = std::make_unique<std::thread>([this]() {
			if (SUCCEEDED(mModel->Set_Current_Time(Unix_Time_To_Rat_Time(time(nullptr))))) {
				mModel->Step(0.0);	//emit the initial state as this is the current state now
				while (!mQuitting) {
					if (!SUCCEEDED(mModel->Step(mFixed_Stepping))) break;

					mTotal_Time += mFixed_Stepping;
					if (mMax_Time > 0.0) mQuitting |= mTotal_Time >= mMax_Time;
				}

				if ((mTotal_Time >= mMax_Time) && mEmit_Shutdown) {
					auto evt = glucose::UDevice_Event{ glucose::NDevice_Event_Code::Shut_Down };
					glucose::IDevice_Event *raw_event = evt.get();
					evt.release();
					mModel->Execute(raw_event);
				}
			}

		});
	}

	return S_OK;
}

HRESULT IfaceCalling CSignal_Generator::QueryInterface(const GUID* riid, void** ppvObj)
{
	if (Internal_Query_Interface<glucose::IFilter_Feedback_Receiver>(glucose::IID_Filter_Feedback_Receiver, *riid, ppvObj)) return S_OK;
	return E_NOINTERFACE;
}

HRESULT IfaceCalling CSignal_Generator::Name(wchar_t** const name) {
	if (mFeedback_Name.empty()) return E_INVALIDARG;

	*name = const_cast<wchar_t*>(mFeedback_Name.c_str());
	return S_OK;
}