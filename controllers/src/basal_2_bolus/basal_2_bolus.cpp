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

#include "basal_2_bolus.h"

CBasal_2_Bolus::CBasal_2_Bolus(scgms::IFilter *output) : scgms::CBase_Filter(output) {
}

CBasal_2_Bolus::~CBasal_2_Bolus() {
	
}

bool CBasal_2_Bolus::Schedule_Delivery(const double current_device_time, const double target_rate, const uint64_t segment_id) {
	bool result = false;

	if (target_rate > 0.0) {
		const double old_delivery_per_period = mInsulin_To_Deliver_Per_Period;

		mInsulin_To_Deliver_Per_Period = target_rate * mParameters.period / scgms::One_Hour;
		mEffective_Period = mParameters.period;

		const double ratio = mParameters.minimum_amount / mInsulin_To_Deliver_Per_Period;
		if (ratio > 1.0) {
			//current volume is too small => let's prolong the period and enlarge the amount
			mEffective_Period = mParameters.period * ratio;
			mInsulin_To_Deliver_Per_Period *= ratio;
		}


		if (!mValid_Settings) {
			mNext_Delivery_Time = current_device_time;		//we start delivering now
			mValid_Settings = true;
			result = true;
		}
		else {
			const double delta = mInsulin_To_Deliver_Per_Period - old_delivery_per_period;
			if (delta > mParameters.minimum_amount) {
				//let's us just issue a compensation dose
				scgms::UDevice_Event event{ scgms::NDevice_Event_Code::Level };
				event.device_time() = current_device_time;
				event.level() = delta;
				event.signal_id() = scgms::signal_Requested_Insulin_Bolus;
				event.device_id() = basal_2_bolus::filter_id;
				event.segment_id() = segment_id;

				result = Succeeded(mOutput.Send(event));
			}
			else
				result = true;
			//else we deliver on the original delivery time not to generate excess insulin doses
		}		
	}
	else {
		//let's stop the insulin deliveries
		mValid_Settings = false;
		result = target_rate == 0.0;	//let's report negative rates as errors
	}


	return result;
}


HRESULT CBasal_2_Bolus::Deliver_Bolus(const double delivery_device_time, const uint64_t segment_id) {	

	scgms::UDevice_Event event{ scgms::NDevice_Event_Code::Level };
	if (event) {
		event.device_time() = delivery_device_time;
		event.level() = mInsulin_To_Deliver_Per_Period;
		event.signal_id() = scgms::signal_Requested_Insulin_Bolus;
		event.device_id() = basal_2_bolus::filter_id;
		event.segment_id() = segment_id;


		mNext_Delivery_Time = delivery_device_time + mEffective_Period;

		return mOutput.Send(event);
	}
	else
		return E_OUTOFMEMORY;
}

HRESULT CBasal_2_Bolus::Do_Execute(scgms::UDevice_Event event) {
	const double current_device_time = event.device_time();
	

	if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate) {
		if (!Schedule_Delivery(current_device_time, event.level(), event.segment_id()))
			return E_INVALIDARG;

		//we are set, hence we change the signal to the delivered insulin basal rate
		event.signal_id() = scgms::signal_Delivered_Insulin_Basal_Rate;
	}
	
	if (event.segment_id() != scgms::Invalid_Segment_Id) {
		if ((current_device_time >= mNext_Delivery_Time) && (mValid_Settings)) {
			const HRESULT rc = Deliver_Bolus(current_device_time, event.segment_id());
			if (!Succeeded(rc))
				return rc;
		}
	}



	return mOutput.Send(event);

}


HRESULT CBasal_2_Bolus::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	std::vector<double> lower, def, upper;
	if (!configuration.Read_Parameters(rsParameters, lower, def, upper) || 
		 (def.size() != basal_2_bolus::model_param_count) ||
		(def[0] <=0.0) || (def[1]<=0.0) ) {	

		error_description.push(dsStored_Parameters_Corrupted_Not_Loaded);
		return E_INVALIDARG;
	}

	mParameters = *reinterpret_cast<basal_2_bolus::TParameters*>(def.data());

	return S_OK;
}