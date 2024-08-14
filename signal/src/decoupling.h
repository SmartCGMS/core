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

#pragma once

#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/FilesystemLib.h>

#include "expression/expression.h"

#include <map>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for mapping input signal GUID to another
 */
class CDecoupling_Filter : public virtual scgms::CBase_Filter {
	protected:
		// source signal ID (what signal will be mapped)
		GUID mSource_Id = Invalid_GUID;
		// destination signal ID (to what ID it will be mapped)
		GUID mDestination_Id = Invalid_GUID;
		bool mRemove_From_Source = false;
		bool mAny_Source_Signal = false;
		bool mDestination_Null = false;
		bool mCollect_Statistics = false;
		filesystem::path mCSV_Path;

		CExpression mCondition;

		//stats
		struct TSegment_Stats {
			size_t events_evaluated = 0, levels_evaluated = 0, events_matched = 0, levels_matched = 0;
			std::vector<double> episode_period;
			std::vector<double> episode_levels;     //not size_t, but double so that we can call statistics routines on it easily
			double current_episode_start_time = std::numeric_limits<double>::quiet_NaN();
			size_t current_episode_levels = 0;
			double recent_device_time = std::numeric_limits<double>::quiet_NaN();
			bool in_episode = false;
		};

		std::map<uint64_t, TSegment_Stats> mStats;

	protected:
		void Update_Stats(scgms::UDevice_Event& event, bool condition_true);
		void Flush_Stats();

		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CDecoupling_Filter(scgms::IFilter *output);
		virtual ~CDecoupling_Filter() = default;
};

#pragma warning( pop )
