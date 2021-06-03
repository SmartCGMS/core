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

#include "pattern_prediction_descriptor.h"
#include "pattern_prediction_data.h"

#include <map>
#include <array>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CPattern_Prediction_Filter : public virtual scgms::CBase_Filter {
protected:
    size_t Level_2_Band_Index(const double level);
protected:
    enum NClassify : size_t {
        pattern = 0,
        band = 1,
        success = 2,
    };

    using TClassification = std::tuple<pattern_prediction::NPattern, size_t, bool>;
    TClassification Classify(scgms::SSignal& ist, const double current_time);
protected:
    double mDt = 30.0 * scgms::One_Minute;

    std::map<uint64_t, scgms::SSignal> mIst;	//ist signals per segments
    std::array<std::array<CPattern_Prediction_Data, pattern_prediction::Band_Count>,
                                                    static_cast<size_t>(pattern_prediction::NPattern::count)> mPatterns;
    
    double Update_And_Predict(const uint64_t segment_id, const double current_time, const double current_level);   
    void Update_Learn(scgms::SSignal &ist, const double current_time, const double current_ig_level);
    double Predict(scgms::SSignal &ist, const double current_time);
protected:
    bool mDo_Not_Learn = false;    
    bool mUpdate_Parameters_File = true;
    bool mUse_Config_Parameters = false;
    filesystem::path mParameters_File_Path;

    bool mUpdated_Levels = false;   //flag to eliminate unnecessary writes to the parameters file

    const wchar_t* isPattern = L"Pattern_";
    const wchar_t* isBand = L"_Band_";
    const wchar_t* iiState = L"State";

    HRESULT Read_Parameters_File(refcnt::Swstr_list error_description);
    HRESULT Read_Parameters_From_Config(scgms::SFilter_Configuration configuration, refcnt::Swstr_list error_description);
    void Write_Parameters_File();
protected:
    // scgms::CBase_Filter iface implementation
    virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
    virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
    CPattern_Prediction_Filter(scgms::IFilter* output);
    virtual ~CPattern_Prediction_Filter();
};

#pragma warning( pop )