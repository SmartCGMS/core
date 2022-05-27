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

#pragma once

#include "../../../../common/rtl/FilterLib.h"
#include "../descriptor.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CBase_Functions_Predictor : public scgms::CBase_Filter, public scgms::IDiscrete_Model
{
	protected:
		bases_pred::TParameters mParameters;

		double mCurrent_Time;
		uint64_t mSegment_Id;

		double mLast_Activated_Day = -1;

		struct Stored_Signal {
			double time_start, time_end;
			double value;
		};

		std::vector<Stored_Signal> mStored_IG;
		std::list<Stored_Signal> mStored_Insulin;
		std::list<Stored_Signal> mStored_Carbs;
		double mLast_Physical_Activity_Index = 0;
		double mLast_Physical_Activity_Time = 0;

		struct TActive_Base
		{
			size_t idx;
			double toff;
		};

		std::list<TActive_Base> mActive_Bases;

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

		double Calc_IOB_At(const double time);
		double Calc_COB_At(const double time);

	public:
		CBase_Functions_Predictor(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output);
		virtual ~CBase_Functions_Predictor() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
