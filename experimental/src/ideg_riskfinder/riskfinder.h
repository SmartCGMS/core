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

#pragma once

#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/referencedImpl.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

 /*
  * Filter class for finding risky episodes in replayed data
  */
class CRisk_Finder_Filter : public virtual scgms::CBase_Filter {
	private:
		bool mFind_Hypo = true;
		bool mFind_Hyper = true;

		double mLast_Hypo_Start = std::numeric_limits<double>::quiet_NaN();
		double mLast_Hyper_Start = std::numeric_limits<double>::quiet_NaN();

		double mHypo_Avg = 0;
		size_t mHypo_Cnt = 0;
		double mHyper_Avg = 0;
		size_t mHyper_Cnt = 0;

		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;

	protected:
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

		bool Emit_Risk_Level(double duration, double time, const GUID& signal_id);
		bool Emit_Risk_Avg_Level(double avg, double time);

	public:
		CRisk_Finder_Filter(scgms::IFilter* output);
		virtual ~CRisk_Finder_Filter() {};
};

#pragma warning( pop )
