/* Examples and Documentation for
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
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "basal_and_bolus.h"
#include "descriptor.h"

#include "../../../common/rtl/SolverLib.h"

HRESULT CBasal_And_Bolus::Do_Execute(scgms::UDevice_Event event) {
	if (!mBasal_Rate_Issued) {
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
		evt.signal_id() = scgms::signal_Requested_Insulin_Basal_Rate;
		evt.device_id() = icarus_v1_boluses::model_id;
		evt.device_time() = event.device_time();
		evt.segment_id() = mSegment_id;
		evt.level() = mParameters.basal_rate;
		mBasal_Rate_Issued = Succeeded(mOutput.Send(evt));
	}

	if ((event.signal_id() == scgms::signal_Carb_Intake) &&
		(event.event_code() == scgms::NDevice_Event_Code::Level)) {
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
		evt.signal_id() = scgms::signal_Requested_Insulin_Bolus;
		evt.device_id() = icarus_v1_boluses::model_id;
		evt.device_time() = event.device_time();
		evt.segment_id() = mSegment_id;
		evt.level() = mParameters.carb_to_insulin * event.level();
		HRESULT rc = mOutput.Send(evt);

		if (!Succeeded(rc))
			return rc;
	}

	return mOutput.Send(event);
}

HRESULT CBasal_And_Bolus::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	return S_OK;
}

HRESULT IfaceCalling CBasal_And_Bolus::Initialize(const double new_current_time, const uint64_t segment_id) {
	mSegment_id = segment_id;
	mBasal_Rate_Issued = false;
	
	return S_OK;
}

HRESULT IfaceCalling CBasal_And_Bolus::Step(const double time_advance_delta) {
	return S_OK;
}