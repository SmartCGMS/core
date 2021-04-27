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
}

void CPattern_Prediction_Data::Update(const double level) {
    if (Is_Any_NaN(level)) return;
       

    mState.push_back(level);
    if (mState.size() > mState_Size)
        mState.erase(mState.begin());
}

double CPattern_Prediction_Data::Level() const {   
    if (mState.size() == 1) return mState[0];
    if (mState.size() == 2) return 0.5*(mState[0]+mState[1]);

    auto [min_level, max_level] = std::minmax_element(mState.begin(), mState.end());

    double best_sum = std::numeric_limits<double>::max();
    double best_p95 = std::numeric_limits<double>::max();
    double best_level = *min_level;
    double level = *min_level;
    

    size_t p95_cnt = std::max(static_cast<size_t>(static_cast<double>(mState.size())*0.95), static_cast<size_t>(1));
    const double p95_th = 0.15;

    while (level <= *max_level) {

        std::vector<double> errors;
        for (const auto& e : mState) {
            const double re = fabs(e - level) / e;
            errors.push_back(re);
         
        }

        std::sort(errors.begin(), errors.end());
        errors.resize(p95_cnt);


        double invn = static_cast<double>(errors.size());
        //first, try Unbiased estimation of standard deviation
       // if (invn > 1.5) invn -= 1.5;
      //  else if (invn > 1.0) invn -= 1.0;	//if not, try to fall back to Bessel's Correction at least
        invn = 1.0 / (invn-1.5);




        double sum = 0.0;
        for (const auto& e : errors) {
            sum += e;
        }

        double avg = sum * invn;
        sum = 0.0;
        for (const auto& e : errors) {
            const double tmp = avg - e;
            sum += tmp * tmp;
        }

        avg += sqrt(sum * invn);
        if (avg < best_sum) {
            best_sum = avg;
            best_level = level;
        }
        


        
        /*std::sort(errors.begin(), errors.end());
        errors.resize(p95_cnt);

        if (best_p95 > p95_th) {
            //we need to fit 95% of all errors below 30%
            if (errors[p95_cnt - 1] < best_p95) {
                best_p95 = errors[p95_cnt - 1];
                best_sum = std::accumulate(errors.begin(), errors.end(), 0.0);
                best_level = level;
            }
        }
        else if ((best_p95 <= p95_th) && (errors[p95_cnt - 1] <= p95_th)) {
            const auto local_sum = std::accumulate(errors.begin(), errors.end(), 0.0);
            if (local_sum < best_sum) {
                best_p95 = errors[p95_cnt - 1];
                best_sum = local_sum;
                best_level = level;
            }
        }
        */
        

        level += mStepping;
    }

    return best_level;   
}

bool CPattern_Prediction_Data::Valid() const {
    return mState.size() > 0;
}


void CPattern_Prediction_Data::Set_State(const double& level) {
    mState.clear();
    for (size_t i = 0; i < mState_Size; i++)
        mState.push_back(level);
}

void CPattern_Prediction_Data::State_from_String(const std::wstring& state) {
    std::wstring str_copy{ state };	//wcstok modifies the input string
    const wchar_t* delimiters = L" ";	//string of chars, which designate individual delimiters
    wchar_t* buffer = nullptr;
    wchar_t* str_val = wcstok_s(const_cast<wchar_t*>(str_copy.data()), delimiters, &buffer);
    
    while ((str_val != nullptr) && (mState.size()< mState_Size)) {
         //and store the real value
         bool ok;
         const double value = str_2_dbl(str_val, ok);
         if (ok) mState.push_back(value);
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
    for (size_t i = 0; i<mState.size(); i++) {
        if (not_empty)
            converted << L" ";
        else
            not_empty = true;

        converted << mState[i];
    }

    return converted.str();
}