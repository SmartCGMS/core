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

#include "rates_pack_boluses.h"
#include "descriptor.h"

#include <scgms/rtl/SolverLib.h>
#include <scgms/utils/DebugHelper.h>

CRates_Pack_Boluses::CRates_Pack_Boluses(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output) :
	CBase_Filter(output, rates_pack_boluses::model_id),
	scgms::CDiscrete_Model<rates_pack_boluses::TParameters>(parameters, rates_pack_boluses::default_parameters, output, rates_pack_boluses::model_id) {	
}

CRates_Pack_Boluses::~CRates_Pack_Boluses() {

}

void CRates_Pack_Boluses::Check_Insulin_Delivery(const double current_time) {
	if (!mBasal_Rate_Issued) {
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
		evt.signal_id() = scgms::signal_Requested_Insulin_Basal_Rate;
		evt.device_id() = rates_pack_boluses::model_id;
		evt.device_time() = mStepped_Current_Time;
		evt.segment_id() = mSegment_id;
		evt.level() = mParameters.initial_basal_rate;
		mBasal_Rate_Issued = Succeeded(mOutput.Send(evt));
	}

	mLGS_Active &= current_time < mLGS_endtime;

	if (!mLGS_Active) {
		Deliver_Insulin<rates_pack_boluses::bolus_max_count>(current_time, mNext_Bolus_Delivery_Time,  mBolus_Index, mParameters.boluses, scgms::signal_Requested_Insulin_Bolus);
		Deliver_Insulin<rates_pack_boluses::basal_change_max_count>(current_time, mNext_Basal_Change_Time, mBasal_Index, mParameters.rates, scgms::signal_Requested_Insulin_Basal_Rate);
	}
}

HRESULT CRates_Pack_Boluses::Do_Execute(scgms::UDevice_Event event) {

	if (event.signal_id() == scgms::signal_IG) {
		const bool lgs_already_active = mLGS_Active;
		mLGS_Active = event.level() <= mParameters.lgs_threshold;
		if (mLGS_Active) {
			mLGS_endtime = event.device_time() + mParameters.lgs_timeout;

			if (!lgs_already_active) {
				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = scgms::signal_Requested_Insulin_Basal_Rate;
				evt.device_id() = rates_pack_boluses::model_id;
				evt.device_time() = event.device_time();
				evt.segment_id() = mSegment_id;
				evt.level() = 0.0;

				HRESULT rc = mOutput.Send(evt);

				if (Succeeded(rc)) {
					return rc;
				}
			}
		}
	}

	return mOutput.Send(event);
}

HRESULT CRates_Pack_Boluses::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	return S_OK;
}

HRESULT IfaceCalling CRates_Pack_Boluses::Initialize(const double new_current_time, const uint64_t segment_id) {
	mStepped_Current_Time = mSimulation_Start = new_current_time;
	mBolus_Index = 0;
	mBasal_Index = 0;
	mSegment_id = segment_id;
	mBasal_Rate_Issued = false;
	mLGS_Active = false;

	mNext_Bolus_Delivery_Time = new_current_time + mParameters.boluses[mBolus_Index].offset;
	mNext_Basal_Change_Time = new_current_time + mParameters.rates[mBasal_Index].offset;
	
	return S_OK;
}

HRESULT IfaceCalling CRates_Pack_Boluses::Step(const double time_advance_delta) {

	if (time_advance_delta > 0.0) {
		// advance the timestamp
		mStepped_Current_Time += time_advance_delta;
		Check_Insulin_Delivery(mStepped_Current_Time);
	}

	return S_OK;
}
