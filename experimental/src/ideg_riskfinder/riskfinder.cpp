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

#include "riskfinder.h"
#include "../descriptor.h"

#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/utils/math_utils.h"

CRisk_Finder_Filter::CRisk_Finder_Filter(scgms::IFilter* output) : CBase_Filter(output) {
	//
}


HRESULT IfaceCalling CRisk_Finder_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mFind_Hypo = configuration.Read_Bool(ideg::riskfinder::rsFind_Hypo, mFind_Hypo);
	mFind_Hyper = configuration.Read_Bool(ideg::riskfinder::rsFind_Hyper, mFind_Hyper);

	return S_OK;
}

HRESULT IfaceCalling CRisk_Finder_Filter::Do_Execute(scgms::UDevice_Event event) {

	// 22 minutes to round up for sensor errors and random spikes
	constexpr double HypoMinTime = scgms::One_Minute * 22;
	constexpr double HyperMinTime = scgms::One_Minute * 11;

	if (event.event_code() == scgms::NDevice_Event_Code::Level && event.signal_id() == scgms::signal_IG) {

		const double level = event.level();

		if (mFind_Hypo) {
			if (level < 4.0) {
				if (Is_Any_NaN(mLast_Hypo_Start)) {
					mLast_Hypo_Start = event.device_time();
					mSegment_Id = event.segment_id();

					mHypo_Avg = event.level();
					mHypo_Cnt = 1;
				}
			}
			else if (!Is_Any_NaN(mLast_Hypo_Start)) {

				mHypo_Avg = (mHypo_Avg*static_cast<double>(mHypo_Cnt) + event.level()) / static_cast<double>(mHypo_Cnt + 1);
				mHypo_Cnt++;

				// hypo lasting longer than 10 minutes
				if ((event.device_time() - mLast_Hypo_Start) > HypoMinTime) {
					Emit_Risk_Level((event.device_time() - mLast_Hypo_Start) / scgms::One_Minute, mLast_Hypo_Start, ideg::riskfinder::risk_hypo_signal_id);
					Emit_Risk_Avg_Level(mHypo_Avg, mLast_Hypo_Start);
				}
				mLast_Hypo_Start = std::numeric_limits<double>::quiet_NaN();
			}
		}

		if (mFind_Hyper) {
			if (level > 10.0) {
				if (Is_Any_NaN(mLast_Hyper_Start)) {
					mLast_Hyper_Start = event.device_time();
					mSegment_Id = event.segment_id();

					mHyper_Avg = event.level();
					mHyper_Cnt = 1;
				}
			}
			else if (!Is_Any_NaN(mLast_Hyper_Start)) {

				mHyper_Avg = (mHyper_Avg * static_cast<double>(mHyper_Cnt) + event.level()) / static_cast<double>(mHyper_Cnt + 1);
				mHyper_Cnt++;

				// hyper lasting longer than 10 minutes
				if ((event.device_time() - mLast_Hyper_Start) > HyperMinTime) {
					Emit_Risk_Level((event.device_time() - mLast_Hyper_Start) / scgms::One_Minute, mLast_Hyper_Start, ideg::riskfinder::risk_hyper_signal_id);
					Emit_Risk_Avg_Level(mHyper_Avg, mLast_Hyper_Start);
				}
				mLast_Hyper_Start = std::numeric_limits<double>::quiet_NaN();
			}
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down) {
		if (!Is_Any_NaN(mLast_Hypo_Start)) {
			// hypo lasting longer than 10 minutes
			if ((event.device_time() - mLast_Hypo_Start) > HypoMinTime) {
				Emit_Risk_Level((event.device_time() - mLast_Hypo_Start) / scgms::One_Minute, mLast_Hypo_Start, ideg::riskfinder::risk_hypo_signal_id);
				Emit_Risk_Avg_Level(mHypo_Avg, mLast_Hypo_Start);
			}
			mLast_Hypo_Start = std::numeric_limits<double>::quiet_NaN();
		}
		if (!Is_Any_NaN(mLast_Hyper_Start)) {
			// hyper lasting longer than 10 minutes
			if ((event.device_time() - mLast_Hyper_Start) > HyperMinTime) {
				Emit_Risk_Level((event.device_time() - mLast_Hyper_Start) / scgms::One_Minute, mLast_Hyper_Start, ideg::riskfinder::risk_hyper_signal_id);
				Emit_Risk_Avg_Level(mHyper_Avg, mLast_Hyper_Start);
			}
			mLast_Hyper_Start = std::numeric_limits<double>::quiet_NaN();
		}
	}

	return mOutput.Send(event);
}

bool CRisk_Finder_Filter::Emit_Risk_Level(double duration, double time, const GUID& signal_id)
{
	scgms::UDevice_Event evt(scgms::NDevice_Event_Code::Level);
	evt.device_id() = ideg::riskfinder::id;
	evt.signal_id() = signal_id;
	evt.level() = duration;
	evt.device_time() = time;
	evt.segment_id() = mSegment_Id;
	return Succeeded(mOutput.Send(evt));
}

bool CRisk_Finder_Filter::Emit_Risk_Avg_Level(double avg, double time)
{
	scgms::UDevice_Event evt(scgms::NDevice_Event_Code::Level);
	evt.device_id() = ideg::riskfinder::id;
	evt.signal_id() = ideg::riskfinder::risk_avg_signal_id;
	evt.level() = avg;
	evt.device_time() = time;
	evt.segment_id() = mSegment_Id;
	return Succeeded(mOutput.Send(evt));
}
