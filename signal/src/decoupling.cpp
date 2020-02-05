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

#include "decoupling.h"

#include  "expression/expression.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"

#include <cmath>


CDecoupling_Filter::CDecoupling_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}



HRESULT IfaceCalling CDecoupling_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mSource_Id = configuration.Read_GUID(rsSignal_Source_Id);
	mDestination_Id = configuration.Read_GUID(rsSignal_Destination_Id);
    mDestination_Null = mDestination_Id == scgms::signal_Null;
    mRemove_From_Source = configuration.Read_Bool(rsRemove_From_Source, mRemove_From_Source);     

    mCondition = Parse_AST_Tree(configuration.Read_String(rsCondition), error_description);
     
	return mCondition ? S_OK : E_FAIL;
}

HRESULT IfaceCalling CDecoupling_Filter::Do_Execute(scgms::UDevice_Event event) {
    switch (event.event_code()) {
        case scgms::NDevice_Event_Code::Warm_Reset: 
            mStats.clear();
            break;

        case scgms::NDevice_Event_Code::Shut_Down:
            Flush_Stats();
            break;

        default: break;

    }


    if (event.signal_id() == mSource_Id) {
        const bool decouple = mCondition->evaluate(event).bval;             
        //cannot test mDestination_Null here, because we would be unable to collect statistics about the expression and release the event if needed

        //clone and signal_Null means gather statistics only
        Update_Stats(event, decouple);

        if (decouple) {
            if (mRemove_From_Source) {

                if (mDestination_Null) {
                    event.reset(nullptr);
                    return S_OK;
                } else 
                    //just change the signal id
                    event.signal_id() = mDestination_Id;
                    //and send it
            }
            else if (!mDestination_Null) {
                //we have to clone it
                auto clone = event.Clone();
                clone.signal_id() = mDestination_Id;
                HRESULT rc = Send(clone);
                if (!SUCCEEDED(rc)) return rc;
            }
        }          
    } 
	  
    return Send(event);		
}

void CDecoupling_Filter::Update_Stats(scgms::UDevice_Event &event, bool condition_true) {
    auto segment = mStats.find(event.segment_id());
    if (segment != mStats.end()) {
        TSegment_Stats stats;
        stats.events_evaluated = stats.levels_evaluated = stats.events_matched = stats.levels_matched = 0;
        stats.episode_period.clear();
        stats.in_episode = false;             
        segment = mStats.insert(mStats.end(), std::pair<uint64_t, TSegment_Stats>{ event.segment_id(), stats });
    }

    auto& stats = segment->second;
    stats.events_evaluated++;
    if (event.event_code() == scgms::NDevice_Event_Code::Level) stats.levels_evaluated++;
   

    if (condition_true) {
    }
    else {
    }
        //at least, we will store new entry for this segment
        auto update_segment = [](TSegment_Stats &stats) {
        

        };

        auto segment = mStats.find(event.segment_id());
        if (segment != mStats.end()) {
            TSegment_Stats stats;
            stats.events_evaluated = stats.levels_evaluated = stats.events_matched = stats.levels_matched = 0;
            stats.episode_period.clear();
            stats.in_episode = false;

            update_segment(stats);
            mStats[event.segment_id()] = std::move(stats);
        } else
            update_segment(segment->second);


    } else if (event.event_code() == scgms::NDevice_Event_Code::Level) { 
        auto segment = mStats.find(event.segment_id());
        if (segment != mStats.end()) {
            auto& stats = segment->second;
            if (stats.in_episode) {
                stats.in_episode = false;
                stats.episode_period.push_back(event.device_time()-stats.current_episode_start_time);
            }
        }

    }
}

void CDecoupling_Filter::Flush_Stats() {

}
