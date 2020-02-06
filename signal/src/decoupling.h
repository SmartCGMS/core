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

#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

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
    bool mDestination_Null = false;
    bool mCollect_Statistics = false;
    std::wstring mCSV_Path;

    CExpression mCondition;
protected:
    //stats
    struct TSegment_Stats {
        size_t events_evaluated, levels_evaluated, events_matched, levels_matched;
        std::vector<double> episode_period;
        std::vector<double> episode_levels;     //not size_t, but double so that we can call statistics routines on it easily
        double current_episode_start_time;
        size_t current_episode_levels;
        double recent_device_time;
        bool in_episode;
    };

    std::map<uint64_t, TSegment_Stats> mStats;

    void Update_Stats(scgms::UDevice_Event& event, bool condition_true);
    void Flush_Stats();
protected:
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
    CDecoupling_Filter(scgms::IFilter *output);
	virtual ~CDecoupling_Filter() {};
};

#pragma warning( pop )
