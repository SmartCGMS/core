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

#include "decoupling.h"

#include  "expression/expression.h"

#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/utils/math_utils.h"
#include "../../../common/lang/dstrings.h"

#include <cmath>
#include <fstream>


CDecoupling_Filter::CDecoupling_Filter(scgms::IFilter* output) : CBase_Filter(output) {
    //
}



HRESULT IfaceCalling CDecoupling_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
    mSource_Id = configuration.Read_GUID(rsSignal_Source_Id);
    mDestination_Id = configuration.Read_GUID(rsSignal_Destination_Id);
    if (Is_Invalid_GUID(mDestination_Id)) return E_INVALIDARG; //mSource_Id can be Invalid_GUID due to external reasons, such as malformed CSV log events


    mAny_Source_Signal = mSource_Id == scgms::signal_All;
    mDestination_Null = mDestination_Id == scgms::signal_Null;    
    mRemove_From_Source = configuration.Read_Bool(rsRemove_From_Source, mRemove_From_Source);

    mCondition = Parse_AST_Tree(configuration.Read_String(rsCondition), error_description);

    mCollect_Statistics = configuration.Read_Bool(rsCollect_Statistics, mCollect_Statistics);
    mCSV_Path = configuration.Read_File_Path(rsOutput_CSV_File);

    if (mCollect_Statistics && mCSV_Path.empty()) {
        error_description.push(dsOutput_to_file_enabled_but_no_filename_given);
        return E_INVALIDARG;
    }

    return mCondition ? S_OK : E_FAIL;
}

HRESULT IfaceCalling CDecoupling_Filter::Do_Execute(scgms::UDevice_Event event) {
    if (mCollect_Statistics) {
        switch (event.event_code()) {
        case scgms::NDevice_Event_Code::Warm_Reset:
            mStats.clear();
            break;

        case scgms::NDevice_Event_Code::Shut_Down:
            Flush_Stats();
            break;

        default: break;
        }
    }


    if ((event.signal_id() == mSource_Id) || mAny_Source_Signal) {
        const bool decouple = mCondition->evaluate(event).bval;
        //cannot test mDestination_Null here, because we would be unable to collect statistics about the expression and release the event if needed

        //clone and signal_Null means gather statistics only
        if (mCollect_Statistics) Update_Stats(event, decouple);

        if (decouple) {
            if (mRemove_From_Source) {

                if (mDestination_Null) {
                    event.reset(nullptr);
                    return S_OK;
                }
                else
                    //just change the signal id
                    event.signal_id() = mDestination_Id;
                //and send it
            }
            else if (!mDestination_Null) {
                //we have to clone it
                auto clone = event.Clone();
                clone.signal_id() = mDestination_Id;
                HRESULT rc = mOutput.Send(clone);
                if (!Succeeded(rc)) return rc;
            }
        }
    }

    return mOutput.Send(event);
}

void CDecoupling_Filter::Update_Stats(scgms::UDevice_Event& event, bool condition_true) {
    auto segment = mStats.find(event.segment_id());
    if (segment == mStats.end()) {
        TSegment_Stats stats;
        stats.events_evaluated = stats.levels_evaluated = stats.events_matched = stats.levels_matched = 0;
        stats.episode_period.clear();
        stats.in_episode = false;
        stats.recent_device_time = std::numeric_limits<double>::quiet_NaN();
        stats.current_episode_levels = 0;
        segment = mStats.insert(mStats.end(), std::pair<uint64_t, TSegment_Stats>{ event.segment_id(), stats });
    }

    auto& stats = segment->second;
    stats.events_evaluated++;
    const bool is_level = event.event_code() == scgms::NDevice_Event_Code::Level;
    if (is_level) stats.levels_evaluated++;


    if (condition_true) {
        stats.events_matched++;

        if (is_level) {
            stats.levels_matched++;
            stats.current_episode_levels++;

            if (!stats.in_episode) {
                //estimate start of the episode
                if (std::isnan(stats.recent_device_time)) stats.current_episode_start_time = event.device_time();
                else stats.current_episode_start_time = 0.5 * (stats.recent_device_time + event.device_time()); //see the reasoning below

            }


            stats.in_episode = true;
        }
    }
    else {
        if (stats.in_episode) {
            stats.in_episode = false;

            stats.episode_period.push_back(0.5 * (stats.recent_device_time + event.device_time()) - stats.current_episode_start_time);
            //as the levels are actually sample, thus. discrete, we do not know the exact of episode-state transition 
            //=> we guess the middle time

            stats.episode_levels.push_back(static_cast<double>(stats.current_episode_levels));
            stats.current_episode_levels = 0;

        }
    }

    stats.recent_device_time = event.device_time();
}

void CDecoupling_Filter::Flush_Stats() {
    std::wofstream stats_file{ mCSV_Path.c_str() };
    if (!stats_file.is_open()) {
        Emit_Info(scgms::NDevice_Event_Code::Error, std::wstring{ dsCannot_Open_File }+mCSV_Path.wstring());
        return;
    }

    {   //write the header
        scgms::CSignal_Description signal_names;
        stats_file << signal_names.Get_Name(mDestination_Id) << "; " << GUID_To_WString(mDestination_Id) << std::endl;
        stats_file << rsDecoupling_Stats_Header << std::endl;
    }


    auto flush_marker = [&stats_file](const uint64_t segment_id, auto& series, auto& stats, const wchar_t* marker_string) {
        scgms::TSignal_Stats signal_stats;

        const bool result = Calculate_Signal_Stats(series, signal_stats);

        if (result) {
            using et = std::underlying_type < scgms::NECDF>::type;

            if (segment_id == scgms::All_Segments_Id)  stats_file << dsSelect_All_Segments;
            else stats_file << std::to_wstring(segment_id);

            stats_file << "; " << marker_string << ";; "
                << signal_stats.avg << "; " << signal_stats.stddev << "; " << signal_stats.exc_kurtosis << "; " << signal_stats.skewness << "; " << signal_stats.count << ";; "
                << signal_stats.ecdf[scgms::NECDF::min_value] << "; "
                << signal_stats.ecdf[scgms::NECDF::p25] << "; "
                << signal_stats.ecdf[scgms::NECDF::median] << "; "
                << signal_stats.ecdf[scgms::NECDF::p75] << "; "
                << signal_stats.ecdf[scgms::NECDF::max_value] << ";; "
                << stats.events_matched << "; " << stats.events_evaluated << "; "
                << stats.levels_matched << "; " << stats.levels_evaluated << "; "
                << std::endl;

        }

        return result;
    };


    TSegment_Stats total_stats; //stats across all segments
    total_stats.events_evaluated = total_stats.levels_evaluated = total_stats.events_matched = total_stats.levels_matched = 0;


    //accumulate data for total stats, while flushing partial stats per segments
    for (const auto& segment : mStats) { //signal must be const because we can FlushStats in later implementations sooner than in dtor             
        auto& levels = segment.second;

        total_stats.events_evaluated += levels.events_evaluated;
        total_stats.levels_evaluated += levels.levels_evaluated;
        total_stats.events_matched += levels.events_matched;
        total_stats.levels_matched += levels.levels_matched;


        auto call_flush_marker = [&flush_marker](const size_t segment_id, auto& series, auto& stats, auto& total_series, const wchar_t* marker_string) {
            if (series.size() > 0) {
                std::vector<double> levels_stats{ series };   //we need a copy due to the sort to find ECDF
                if (flush_marker(segment_id, levels_stats, stats, marker_string)) {
                    std::move(series.begin(), series.end(), std::back_inserter(total_series));
                }
            }
        };

        call_flush_marker(segment.first, levels.episode_levels, levels, total_stats.episode_levels, dsLevel);
        call_flush_marker(segment.first, levels.episode_period, levels, total_stats.episode_period, dsPeriod);
    }

    stats_file << std::endl;

    if (flush_marker(scgms::All_Segments_Id, total_stats.episode_levels, total_stats, dsLevel))
        flush_marker(scgms::All_Segments_Id, total_stats.episode_period, total_stats, dsPeriod);
}
