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

#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/rtl/FilterLib.h"

#include "neural_net_descriptor.h"
#include "neural_prediction_pattern.h"

#include <map>
#include <array>
#include <algorithm>

namespace neural_prediction {
    using TLevels = std::array<double, 3>;

    class CSegment_Signals {
    public:
        

        const static size_t ist_idx = 0;
        const static size_t cob_idx = 1;
        const static size_t iob_idx = 2;
        const static size_t invalid = std::numeric_limits<size_t>::max();

        std::array<scgms::SSignal, 3> signals;        

        static size_t Signal_Id_2_Idx(const GUID& id);
    public:
        CSegment_Signals();

        bool Update(const size_t sig_idx, const double time, const double level);
        bool Get_Levels(const TLevels times, TLevels& ist, TLevels& iob, TLevels& cob);
    };
}

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CNN_Prediction_Filter : public virtual scgms::CBase_Filter {
protected:
    using TNN_Input = typename const_neural_net::CNeural_Network::TInput;
    std::map< TNN_Input, CNeural_Prediction_Data,
        std::function<bool(const TNN_Input&, const TNN_Input&)>> mPatterns{ 
        [](const TNN_Input& a, const TNN_Input& b) {
        return std::lexicographical_compare(a.data(),a.data() + a.size(),
                                            b.data(),b.data() + b.size()); } };
        
    size_t mKnown_Counter = 0;
    size_t mUnknown_Counter = 0;
protected:
    double mDt = 30.0 * scgms::One_Minute;
    double mRecent_Predicted_Level = std::numeric_limits<double>::quiet_NaN();
    const_neural_net::CNeural_Network mNeural_Net;
    std::map<uint64_t, neural_prediction::CSegment_Signals> mSignals;
    double Update_And_Predict(const size_t sig_idx, const uint64_t segment_id, const double current_time, const double current_level);   
    typename const_neural_net::CNeural_Network::TInput Prepare_Input(neural_prediction::CSegment_Signals& signals, const double current_time, double &delta);
    void Learn(neural_prediction::CSegment_Signals& signals, const size_t sig_idx, const double current_time, const double current_level);
    double Predict(neural_prediction::CSegment_Signals& signals, const double current_time);

    double Predict_Conservative(neural_prediction::CSegment_Signals& signals, const double current_time);
    double Predict_Poly(neural_prediction::CSegment_Signals& signals, const double current_time);
    double Predict_Akima(neural_prediction::CSegment_Signals& signals, const double current_time);
protected:
    const bool mDump_Params = true;
    void Dump_Params();
protected:
    // scgms::CBase_Filter iface implementation
    virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
    virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
    CNN_Prediction_Filter(scgms::IFilter* output);
    virtual ~CNN_Prediction_Filter();
};

#pragma warning( pop )