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

#include "pattern_prediction_descriptor.h"

#include <array>
#include <tuple>

class CPattern_Prediction_Data {
protected:
    //let there be simple circular buffer    
    static constexpr size_t mState_Size = 40;    
    std::array<double, mState_Size> mState;    
    size_t mHead = 0;
    bool mFull = false; //true if we have filled the entire buffer
                        //and we are overwriting the old values
protected:
    bool mEncountered = false;              //so that we are able to sanitize unused patterns
    bool mCollect_Learning_Data =false;
    struct TLevel { double device_time, level; };
    std::vector<TLevel> mLearning_Data;
protected:
    //prediction helpers
    const double mTrusted_Perimeter = 2.0;
    double mRecent_Prediction = std::numeric_limits<double>::quiet_NaN();
    bool mInvalidated = true;   //true, when ::predict must recalculate mRecent_Prediction
public:
    CPattern_Prediction_Data();


    void push(const double device_time, const double level);
    double predict();
    explicit operator bool() const;

    void Set_State(const double& level);
    void State_from_String(const std::wstring& state);
    std::wstring State_To_String() const;

    void Start_Collecting_Learning_Data();
    std::wstring Learning_Data(const size_t sliding_window_length, const double dt) const;

    void Encounter();
    bool Was_Encountered() const;
};