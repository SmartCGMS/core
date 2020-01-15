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

#include "signal_stats.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/utils/string_utils.h"


#include <fstream>


CSignal_Stats::CSignal_Stats(scgms::IFilter* output) : CBase_Filter(output) {

}

CSignal_Stats::~CSignal_Stats() {
    Flush_Stats();
}

HRESULT CSignal_Stats::Do_Configure(scgms::SFilter_Configuration configuration) {
    mCSV_Path = configuration.Read_String(rsOutput_CSV_File);
    
    return mCSV_Path.empty() ? E_INVALIDARG : S_OK; //change E_INVALIDARG to S_FALSE or S_OK once we implement an inspection iface
}

HRESULT CSignal_Stats::Do_Execute(scgms::UDevice_Event event) {
    auto add_level_to_segment = [&](TSegment_Series &segment) {
        auto series = segment.find(event.segment_id());

        if (series == segment.end()) {
            std::vector<TLevel> level;
            level.push_back(TLevel{event.level(), event.device_time()});
            segment[event.segment_id()] = std::move(level);
        } else
            series->second.push_back(TLevel{ event.level(), event.device_time() });
    };


    if (event.event_code() == scgms::NDevice_Event_Code::Level) {   //we intentionally ignore masked levels

        //1. find the signal
        auto signal = mSignal_Series.find(event.signal_id());
        if (signal == mSignal_Series.end()) {
            TSegment_Series segment;
            add_level_to_segment(segment);
            mSignal_Series[event.signal_id()] = std::move(segment);
        }
        else
            add_level_to_segment(signal->second);
    }

    return S_OK;
}


void CSignal_Stats::Flush_Stats() {
    if (mCSV_Path.empty()) return;

    std::wofstream stats{ Narrow_WChar(mCSV_Path.c_str()) };
    if (!stats.is_open()) return;

    stats << rsSignal_Stats_Header << std::endl;

    scgms::CSignal_Names signal_names;

    for (const auto& signal : mSignal_Series) {

        double level_avg = 0.0;
        double period_avg = 0.0;

        size_t level_count = 0;
        size_t period_count = 0;

        for (auto& segment : signal.second) {
            auto& data = segment.second;
            
        }


        //calculated level statistics
        stats << signal_names.Get_Name(signal.first) << L"_level; " << GUID_To_WString(signal.first) << L"_level;; ";


        //calculate time statistics
        stats << signal_names.Get_Name(signal.first) << L"_period; " << GUID_To_WString(signal.first) << L"_period;; ";
        
    }
}