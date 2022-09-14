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

#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/UILib.h"

#include "descriptor.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance


class CRates_Pack_Boluses : public virtual scgms::CDiscrete_Model<rates_pack_boluses::TParameters> {
protected:
	double mSimulation_Start = std::numeric_limits<double>::quiet_NaN();
	double mStepped_Current_Time = std::numeric_limits<double>::quiet_NaN();
	size_t mBolus_Index = std::numeric_limits<size_t>::max();
	size_t mBasal_Index = std::numeric_limits<size_t>::max();
	uint64_t mSegment_id = scgms::Invalid_Segment_Id;
	bool mBasal_Rate_Issued = false;
	bool mLGS_Active = false;
	double mLGS_endtime = 0.0;

		//delivers insulin bolus, only if its time is LESS-ONLY than the current_time
		//LESS-ONLY let us keep the code short and clean
	void Check_Insulin_Delivery(const double current_time);


	template <size_t max_count, typename T>
	void Deliver_Insulin(const double current_time, size_t &index, const T &values, const GUID &signal_id) {
		const double offset = current_time - mSimulation_Start;

		if (index < max_count) {
			if (offset >= values[index].offset) {

				scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
				evt.signal_id() = signal_id;
				evt.device_id() = rates_pack_boluses::model_id;
				evt.device_time() = current_time;
				evt.segment_id() = mSegment_id;
				evt.level() = values[index].value;

				if (Succeeded(mOutput.Send(evt)))
					index++;
			}
		}
	}
protected:
	// scgms::CBase_Filter iface implementation
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CRates_Pack_Boluses(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output) :
		CBase_Filter(output, rates_pack_boluses::model_id),
		scgms::CDiscrete_Model<rates_pack_boluses::TParameters>(parameters, rates_pack_boluses::default_parameters, output, rates_pack_boluses::model_id)
		{}
	virtual ~CRates_Pack_Boluses();

	// scgms::IDiscrete_Model iface
	virtual HRESULT IfaceCalling Initialize(const double new_current_time, const uint64_t segment_id) override final;
	virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
