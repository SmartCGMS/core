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

#include "signal_generator.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/math_utils.h"
#include "../../../common/utils/string_utils.h"

#include <cmath>

signal_generator_internal::CSynchronized_Generator::CSynchronized_Generator(
	scgms::IFilter *direct_output, scgms::IFilter *chained_output, const uint64_t segment_id)
	: mSegment_Id(segment_id), mDirect_Output(direct_output), mChained_Output(chained_output) {

}

signal_generator_internal::CSynchronized_Generator::~CSynchronized_Generator() {

}

HRESULT IfaceCalling signal_generator_internal::CSynchronized_Generator::Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list* error_description) {
	scgms::SFilter_Configuration shared_configuration = refcnt::make_shared_reference_ext<scgms::SFilter_Configuration, scgms::IFilter_Configuration>(configuration, true);

	mFixed_Stepping = shared_configuration.Read_Double(rsStepping, mFixed_Stepping);
	const GUID model_id = shared_configuration.Read_GUID(rsSelected_Model);
	if (Is_Invalid_GUID(model_id) || Is_Any_NaN(mFixed_Stepping)) return E_INVALIDARG;
	mSync_Signal = shared_configuration.Read_GUID(rsSynchronization_Signal);
	
	std::vector<double> lower, parameters, upper;
	shared_configuration.Read_Parameters(rsParameters, lower, parameters, upper);
	
	mSync_Model = scgms::SDiscrete_Model{ model_id, parameters, this };
	if (!mSync_Model)
		return E_FAIL;

	HRESULT rc = mSync_Model->Configure(configuration, error_description);
	return rc;
}

HRESULT IfaceCalling signal_generator_internal::CSynchronized_Generator::Execute(scgms::IDevice_Event *event) {
	scgms::TDevice_Event *raw_event;
	HRESULT rc = event->Raw(&raw_event);
	
	if (rc == S_OK) {
		const bool chained_send = mChained_Output &&
									((raw_event->event_code == scgms::NDevice_Event_Code::Shut_Down) ||
									(raw_event->segment_id == scgms::All_Segments_Id));

		rc = chained_send ? mChained_Output->Execute(event) : mDirect_Output->Execute(event);
	}

	return rc;
}

HRESULT signal_generator_internal::CSynchronized_Generator::Execute_Sync(scgms::UDevice_Event &event) {
	HRESULT rc = E_UNEXPECTED;

	if (!mCatching_Up) {
		if (event.event_code() == scgms::NDevice_Event_Code::Time_Segment_Start)
			mLast_Device_Time = std::numeric_limits<double>::quiet_NaN();

		bool step_the_model = event.is_level_event() && ((event.signal_id() == mSync_Signal) || (mSync_Signal == scgms::signal_All));
		double dynamic_stepping = 0.0;			//means "emit current state"
		if (step_the_model) {
			if (!std::isnan(mLast_Device_Time)) {
				dynamic_stepping = event.device_time() - mLast_Device_Time;

				if (dynamic_stepping <= 0.0)
					step_the_model = false;
			}
			else {
				//cannot advance the model because this is the very first event, thus we do not have the delta
				mSync_Model->Initialize(event.device_time(), mSegment_Id);	//for which we need to set the current time

				//do not move the initialize from here - if we would replay a historical log, combined
				//with events produced in the present, it could produce wrong dynamic stepping
				//because we nee to lock our time hearbeat on the historical sync_signal, not any signal
			}

			mLast_Device_Time = event.device_time();
		}

		if (step_the_model) {
			if (mFixed_Stepping > 0.0) {
				mCatching_Up = true;
				mTime_To_Catch_Up += dynamic_stepping;
				while (mTime_To_Catch_Up >= mFixed_Stepping) {

					rc = mSync_Model->Step(mFixed_Stepping);
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
		rc = mSync_Model->Execute(raw_event);
		if (!SUCCEEDED(rc))
			return rc;

		if (step_the_model) rc = mSync_Model->Step(dynamic_stepping);
	}
	else {
		//process events we might have triggerd while catching up
		scgms::IDevice_Event *raw_event = event.get();
		event.release();
		rc = mSync_Model->Execute(raw_event);
	}

	return rc;
}

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
	bool shutdown = (event.event_code() == scgms::NDevice_Event_Code::Shut_Down);
	if (mSync_To_Signal) {
		if (shutdown || (event.segment_id() == scgms::All_Segments_Id)) {
			//an event for all segments
			if (mLast_Sync_Generator)
				rc = mLast_Sync_Generator->Execute_Sync(event);
					//the sync'ed generators will subsequenly forward this event among them
			else
				rc = Send(event);
		} else {
			//this event is intended for a single segment only
			auto sync_model_iter = mSync_Models.find(event.segment_id());
			if (sync_model_iter == mSync_Models.end()) {
				TSync_Model sync_model = std::make_unique<signal_generator_internal::CSynchronized_Generator>(mOutput.get(), mLast_Sync_Generator, event.segment_id());
				refcnt::Swstr_list errs;
				rc = sync_model->Configure(mSync_Configuration.get(), errs.get());
				if (!SUCCEEDED(rc)) return rc;

				auto inserted_sync_model = mSync_Models.insert(std::pair<uint64_t, TSync_Model>(event.segment_id(), std::move(sync_model)));
				sync_model_iter = inserted_sync_model.first;
				mLast_Sync_Generator = sync_model_iter->second.get();
			}

			rc = sync_model_iter->second->Execute_Sync(event);
		}

	} else {
		if (mAsync_Model) {

			if (shutdown) 
				Stop_Generator(true);

			scgms::IDevice_Event *raw_event = event.get();
			event.release();
			rc = mAsync_Model->Execute(raw_event);
		}
		else
			rc = ENODEV;
	}

	return rc;
}

HRESULT CSignal_Generator::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	Stop_Generator(true);

	mSync_To_Signal = configuration.Read_Bool(rsSynchronize_to_Signal, mSync_To_Signal);
	mFixed_Stepping = configuration.Read_Double(rsStepping, mFixed_Stepping);
	const GUID model_id = configuration.Read_GUID(rsSelected_Model);
	if (Is_Invalid_GUID(model_id) || Is_Any_NaN(mFixed_Stepping, mMax_Time)) return E_INVALIDARG;
	
	uint64_t segment_id = configuration.Read_Int(rsTime_Segment_ID, scgms::Invalid_Segment_Id);

	const bool seg_id_async_wrong = !mSync_To_Signal && ((segment_id == scgms::Invalid_Segment_Id) || (segment_id == scgms::All_Segments_Id));
	const bool seg_id_sync_wrong = mSync_To_Signal && (segment_id == scgms::Invalid_Segment_Id);
	if (seg_id_async_wrong || seg_id_sync_wrong) {
		error_description.push(dsAsync_Sig_Gen_Req_Seg_Id);
		return E_INVALIDARG;
	}


	if (!mSync_To_Signal && (mFixed_Stepping <= 0.0)) {
		std::wstring str = dsAsync_Stepping_Not_Positive;
		scgms::TModel_Descriptor desc = scgms::Null_Model_Descriptor;
		if (scgms::get_model_descriptor_by_id(model_id, desc))  str += desc.description;
			else str += GUID_To_WString(model_id);
		
		error_description.push(str);
		return E_INVALIDARG;
	}
	
	mFeedback_Name = configuration.Read_String(rsFeedback_Name);	
	mMax_Time = configuration.Read_Double(rsMaximum_Time, mMax_Time);
	mEmit_Shutdown = configuration.Read_Bool(rsShutdown_After_Last, mEmit_Shutdown);

	std::vector<double> lower, parameters, upper;
	configuration.Read_Parameters(rsParameters, lower, parameters, upper);
	mSync_Configuration = configuration.Clone();

	mQuitting = false;

	if (!mSync_To_Signal) {

		mAsync_Model = scgms::SDiscrete_Model{ model_id, parameters, mOutput };
		if (!mAsync_Model)
			return E_FAIL;

		HRESULT rc = mAsync_Model->Configure(configuration.get(), error_description.get());
		if (!SUCCEEDED(rc))
			return rc;

		mThread = std::make_unique<std::thread>([this, segment_id]() {
			double total_time = 0.0;
			
			scgms::SDiscrete_Model model = mAsync_Model; // hold local instance to avoid race conditions with Execute shutdown code
			if (SUCCEEDED(model->Initialize(Unix_Time_To_Rat_Time(time(nullptr)), segment_id))) {
				model->Step(0.0);	//emit the initial state as this is the current state now
				while (!mQuitting) {
					if (!SUCCEEDED(model->Step(mFixed_Stepping))) break;

					total_time += mFixed_Stepping;
					if (mMax_Time > 0.0) {
						if (total_time >= mMax_Time) break;
					}
				}

				if ((total_time >= mMax_Time) && mEmit_Shutdown) {
					auto evt = scgms::UDevice_Event{ scgms::NDevice_Event_Code::Shut_Down };
					scgms::IDevice_Event *raw_event = evt.get();
					evt.release();
					model->Execute(raw_event);
				}
			}
			else
				Emit_Info(scgms::NDevice_Event_Code::Error, dsError_Initializing_Discrete_Model, segment_id);

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
