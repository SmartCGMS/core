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
#include "../../../../common/utils/math_utils.h"
#include "../../../../common/utils/string_utils.h"

#include <cmath>
#include <algorithm>
#include <iomanip>
#include <numeric>

#undef min
#undef max

CPattern_Prediction_Data::CPattern_Prediction_Data() {
    std::fill(mState.begin(), mState.end(), 0.0);
}

void CPattern_Prediction_Data::push(const double level) {
    if (Is_Any_NaN(level)) return;
       
    mState[mHead] = level;
    mInvalidated = true;

    mHead++;    
    if (mHead >= mState.size()) {
        mHead = 0;
        mFull = true;
    }
}


double CPattern_Prediction_Data::predict() {       
    if (mInvalidated) {

        const size_t full_set_n = mFull ? mState.size() : mHead;
        

        double full_set_avg = 0.0;
        for (size_t i = 0; i < full_set_n; i++) {
            full_set_avg += mState[i];
        }
        full_set_avg /= static_cast<double>(full_set_n);    
        
        double trusted_region_avg = 0.0;
        double trusted_region_n = 0.0;

        for (size_t i = 0; i < full_set_n; i++) {
            const double tmp = fabs(mState[i] - full_set_avg);
            if (tmp <= mTrusted_Perimeter) {
                trusted_region_avg += mState[i];
                trusted_region_n += 1.0;
            }
        }

        
        mRecent_Prediction = trusted_region_n > 0.0 ?
            trusted_region_avg / trusted_region_n :
            full_set_avg;
        
        mInvalidated = false;
    }

    return mRecent_Prediction;
}

CPattern_Prediction_Data::operator bool() const {
    return (mHead > 0) || mFull;
}


void CPattern_Prediction_Data::Set_State(const double& level) {    
    for (auto& e : mState)
        e = level;
    
    mHead = 0;
    mFull = true;
    mRecent_Prediction = level;
    mInvalidated = false;
}

void CPattern_Prediction_Data::State_from_String(const std::wstring& state) {
    std::wstring str_copy{ state };	//wcstok modifies the input string
    const wchar_t* delimiters = L" ";	//string of chars, which designate individual delimiters
    wchar_t* buffer = nullptr;
    wchar_t* str_val = wcstok_s(const_cast<wchar_t*>(str_copy.data()), delimiters, &buffer);
    
    while (str_val != nullptr) {
         //and store the real value
         bool ok;
         const double value = str_2_dbl(str_val, ok);
         if (ok) push(value);
         else break;

        str_val = wcstok_s(nullptr, delimiters, &buffer);
    }
}

std::wstring CPattern_Prediction_Data::State_To_String() const {
    std::wstringstream converted;

    //unused keeps static analysis happy about creating an unnamed object
    auto unused = converted.imbue(std::locale(std::wcout.getloc(), new CDecimal_Separator<wchar_t>{ L'.' })); //locale takes owner ship of dec_sep
    converted << std::setprecision(std::numeric_limits<long double>::digits10 + 1);


    bool not_empty = false;

    const size_t full_set_n = mFull ? mState.size() : mHead;

    for (size_t i = 0; i< full_set_n; i++) {
        if (not_empty)
            converted << L" ";
        else
            not_empty = true;

        converted << mState[i];
    }

    return converted.str();
}