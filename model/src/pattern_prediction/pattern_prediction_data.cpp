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

#include "pattern_prediction_data.h"

#include "../../../../common/utils/DebugHelper.h"

#undef min
#undef max

CPattern_Prediction_Data::CPattern_Prediction_Data() {
}

void CPattern_Prediction_Data::Update(const double level) {
    if (std::isnan(level)) return;
       

    mState.count += 1.0;
    if (!std::isnan(mState.running_avg)) {
        const double delta = level - mState.running_avg;
        const double delta_n = delta / mState.count;
        mState.running_avg += delta_n;
        mState.running_median += copysign(mState.running_avg * 0.01, level - mState.running_median);
        mState.running_variance_accumulator += delta * delta_n * (mState.count - 1.0);
    }
    else {
        mState.running_median = mState.running_avg = level;
        mState.running_variance_accumulator = 0.0;
    }
}

double CPattern_Prediction_Data::Level() const {    
    return mState.count > 100.0 ? mState.running_median : mState.running_avg;
}

bool CPattern_Prediction_Data::Valid() const {
    return mState.count > 0.0;
}

TPattern_Prediction_Pattern_State CPattern_Prediction_Data::Get_State() const {
    //first, update the running standard deviation
    
    TPattern_Prediction_Pattern_State result = mState;


    result.running_stddev = result.count > 1.0 ?
                                sqrt(result.running_variance_accumulator / (result.count - 1.0)) :
                                0.0;


    return result;
}

void CPattern_Prediction_Data::Set_State(const TPattern_Prediction_Pattern_State& state) {
    mState = state;
}