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

#include "reference_neural_net.h"

CReference_Neural_Net_Signal::CReference_Neural_Net_Signal(scgms::IFilter* output) : CBase_Filter(output) {
}

HRESULT CReference_Neural_Net_Signal::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
    mDt = configuration.Read_Double(rsDt_Column, mDt);
    return S_OK;
}


HRESULT CReference_Neural_Net_Signal::Do_Execute(scgms::UDevice_Event event) {
    HRESULT rc = E_UNEXPECTED;

    if (event.is_level_event() && (event.signal_id() == scgms::signal_IG)) {        
        const scgms::NDevice_Event_Code code = event.event_code();
        const uint64_t segment_id = event.segment_id();
        const double event_time = event.device_time();
        const double level = event.level();
        rc = Send(event);

        if (SUCCEEDED(rc)) {
            scgms::UDevice_Event ref_event{ code };
            ref_event.signal_id() = neural_net::signal_Neural_Net_Prediction;
            ref_event.level() = neural_net::Histogram_Index_To_Level(neural_net::Level_To_Histogram_Index(level));
            ref_event.device_time() = event_time + mDt;
            ref_event.device_id() = reference_neural_net::filter_id;
            ref_event.segment_id() = segment_id;
            rc = Send(ref_event);
        }
    } else
        rc =  Send(event);     

    return rc;
}
