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

#include "signal_generator.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/math_utils.h"
#include "../../../common/utils/string_utils.h"

#include <cmath>

CSignal_Generator::CSignal_Generator(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CSignal_Generator::~CSignal_Generator() {
	Stop_Generator(true);
}

void CSignal_Generator::Stop_Generator(bool wait) {
	mQuitting = true;
	if (wait && mThread) {
		if (mThread->joinable()) 
			mThread->join();
		mThread.reset();
	}
}

HRESULT CSignal_Generator::Do_Execute(scgms::UDevice_Event event) {

	HRESULT rc = E_UNEXPECTED;

	if (!mCatching_Up && (mSync_To_Signal) && ((mTotal_Time < mMax_Time) || (mMax_Time <= 0.0))) {


		if (event.event_code() == scgms::NDevice_Event_Code::Time_Segment_Start) 
			mLast_Device_Time = std::numeric_limits<double>::quiet_NaN();

		bool step_the_model =  event.is_level_event() && ((event.signal_id() == mSync_Signal) || (mSync_Signal == scgms::signal_All));
		double dynamic_stepping = 0.0;			//means "emit current state"
		if (step_the_model) {
			if (!std::isnan(mLast_Device_Time)) {
				dynamic_stepping = event.device_time() - mLast_Device_Time;
				
				if (dynamic_stepping == 0.0)
					step_the_model = false;
			} else {
																	//cannot advance the model because this is the very first event, thus we do not have the delta
					mModel->Initialize(event.device_time(), reinterpret_cast<std::remove_reference<decltype(event.segment_id())>::type>(mModel.get()));	//for which we need to set the current time
				}

			mLast_Device_Time = event.device_time();
		}

		if (step_the_model) {
			if (mFixed_Stepping > 0.0) {
				mCatching_Up = true;
				mTime_To_Catch_Up += dynamic_stepping;
				while (mTime_To_Catch_Up >= mFixed_Stepping) {

					rc = mModel->Step(mFixed_Stepping);
					if (!SUCCEEDED(rc)) {
						mCatching_Up = false;
						return rc;
					}

					mTime_To_Catch_Up -= mFixed_Stepping;
				}

				step_the_model = (dynamic_stepping == 0.0);	//emits current state
				mCatching_Up = false;
			}
		}

		scgms::IDevice_Event *raw_event = event.get();
		event.release();
		rc = mModel->Execute(raw_event);
		if (!SUCCEEDED(rc))
			return rc;
		
		if (step_the_model) rc = mModel->Step(dynamic_stepping);
	}
	else {

		bool shutdown = (event.event_code() == scgms::NDevice_Event_Code::Shut_Down);

		scgms::IDevice_Event *raw_event = event.get();
		event.release();
		rc = mModel->Execute(raw_event);

		if (shutdown) {
			Stop_Generator(false);
			mModel.reset();
		}
	}

	return rc;
}

HRESULT CSignal_Generator::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	Stop_Generator(true);

	mSync_To_Signal = configuration.Read_Bool(rsSynchronize_to_Signal, mSync_To_Signal);
	mFixed_Stepping = configuration.Read_Double(rsStepping, mFixed_Stepping);
	const GUID model_id = configuration.Read_GUID(rsSelected_Model);
	if (Is_Invalid_GUID(model_id) || Is_NaN(mFixed_Stepping, mMax_Time)) return E_INVALIDARG;


	if (!mSync_To_Signal && (mFixed_Stepping <= 0.0)) {
		std::wstring str = dsAsync_Stepping_Not_Positive;
		scgms::TModel_Descriptor desc = scgms::Null_Model_Descriptor;
		if (scgms::get_model_descriptor_by_id(model_id, desc))  str += desc.description;		
			else str += GUID_To_WString(model_id);
		
		error_description.push(str);
		return E_INVALIDARG;
	}

	mSync_Signal = configuration.Read_GUID(rsSynchronization_Signal);
	mFeedback_Name = configuration.Read_String(rsFeedback_Name);	
	mMax_Time = configuration.Read_Double(rsMaximum_Time, mMax_Time);
	mEmit_Shutdown = configuration.Read_Bool(rsShutdown_After_Last, mEmit_Shutdown);



	std::vector<double> lower, parameters, upper;
	configuration.Read_Parameters(rsParameters, lower, parameters, upper);

	

	mModel = scgms::SDiscrete_Model { model_id, parameters, mOutput };
	if (!mModel)
		return E_FAIL;

	HRESULT rc = mModel->Configure(configuration.get(), error_description.get());
	if (!SUCCEEDED(rc))
		return rc;

	mTotal_Time = 0.0;
	mQuitting = false;

	if (!mSync_To_Signal) {
		mThread = std::make_unique<std::thread>([this]() {
			scgms::SDiscrete_Model model = mModel; // hold local instance to avoid race conditions with Execute shutdown code
			if (SUCCEEDED(model->Initialize(Unix_Time_To_Rat_Time(time(nullptr)), reinterpret_cast<uint64_t>(model.get())))) {
				model->Step(0.0);	//emit the initial state as this is the current state now
				while (!mQuitting) {
					if (!SUCCEEDED(model->Step(mFixed_Stepping))) break;

					mTotal_Time += mFixed_Stepping;
					if (mMax_Time > 0.0) mQuitting |= mTotal_Time >= mMax_Time;
				}

				if ((mTotal_Time >= mMax_Time) && mEmit_Shutdown) {
					auto evt = scgms::UDevice_Event{ scgms::NDevice_Event_Code::Shut_Down };
					scgms::IDevice_Event *raw_event = evt.get();
					evt.release();
					model->Execute(raw_event);
				}
			}

		});
	}

	return S_OK;
}

HRESULT IfaceCalling CSignal_Generator::QueryInterface(const GUID* riid, void** ppvObj)
{
	if (Internal_Query_Interface<scgms::IFilter_Feedback_Receiver>(scgms::IID_Filter_Feedback_Receiver, *riid, ppvObj)) return S_OK;
	return E_NOINTERFACE;
}

HRESULT IfaceCalling CSignal_Generator::Name(wchar_t** const name) {
	if (mFeedback_Name.empty()) return E_INVALIDARG;

	*name = const_cast<wchar_t*>(mFeedback_Name.c_str());
	return S_OK;
}