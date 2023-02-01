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

void CPattern_Prediction_Data::push(const double device_time, const double level) {
    if (Is_Any_NaN(level)) return;

    if (mCollect_Learning_Data) {
        mLearning_Data.push_back({ device_time, level });
    }
       
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

        bool is_mard_small_enough = full_set_n >= 2;
        if (is_mard_small_enough) {

            double full_set_avg = 0.0;
            for (size_t i = 0; i < full_set_n; i++) {               
                full_set_avg += mState[i];
            }
            full_set_avg /= static_cast<double>(full_set_n);
            //full_set_avg provides the best absolute-error fit
            //let's use it, if its MARD does not exceed a specific threshold

            constexpr size_t candidate_count = 8;   //8 to support auto vectorization
            constexpr std::array<double, candidate_count> candidate_offsets = { {0.985, 0.990, 0.995, 1.000, 1.005, 1.010, 1.015, 1.020} };
            std::array<double, candidate_count> candidate_prediction, mards;
            for (size_t i = 0; i < candidate_offsets.size(); i++) {
                candidate_prediction[i] = candidate_offsets[i] * full_set_avg;
                mards[i] = 0.0;
            }

            //compute candidate mard sums - actually, we mimic a more sophisticated solver by computing hard-coded guess solutions
            double MARD_n = 0.0;
            for (size_t i = 0; i < full_set_n; i++) {
                const double current_level = mState[i];
                if (std::isgreater(current_level, 0.0)) {

                    const double inv_current_level = 1.0 / current_level;
                    for (size_t j = 0; j < candidate_prediction.size(); j++)
                        mards[j] += fabs(current_level - candidate_prediction[j]) * inv_current_level;

                    MARD_n += 1.0;
                }
            }

            //find the least sum
            is_mard_small_enough = MARD_n >= 1.9;   //if there are enough data --1.9 accounts rounding errors
            if (is_mard_small_enough) {
                const auto least_mard_iter = std::min_element(mards.begin(), mards.end());
                //check that the least mard is really small enough
                is_mard_small_enough = *least_mard_iter < MARD_n* pattern_prediction::Relative_Error_Correct_Prediction_Threshold;  //multiplication is cheaper

                if (is_mard_small_enough) {
                    mRecent_Prediction = candidate_prediction[std::distance(mards.begin(), least_mard_iter)];
                }
            }
        }

        if (!is_mard_small_enough) 
            mRecent_Prediction = mState[full_set_n - 1];    //relative error is too big, let's reuse the recent level
           

        mInvalidated = false;
    }

    return mRecent_Prediction;
}

double CPattern_Prediction_Data::predict_doi_10_dot_1016_slash_j_compbiomed_dot_2022_dot_105388() {
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
         if (ok) push(0.0, value);
         else break;

        str_val = wcstok_s(nullptr, delimiters, &buffer);
    }
}

std::wstring CPattern_Prediction_Data::State_To_String() const {
    std::wstringstream converted;

    //unused keeps static analysis happy about creating an unnamed object
    auto dec_sep = new CDecimal_Separator<wchar_t>{ L'.' };
    auto unused = converted.imbue(std::locale{std::wcout.getloc(), std::move(dec_sep)}); //locale takes owner ship of dec_sep
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

void CPattern_Prediction_Data::Start_Collecting_Learning_Data() {
    mCollect_Learning_Data = true;
}

std::wstring CPattern_Prediction_Data::Learning_Data(const size_t sliding_window_length, const double dt) const {
    std::wstringstream converted;
    //unused keeps static analysis happy about creating an unnamed object
    auto dec_sep = new CDecimal_Separator<wchar_t>{ L'.' };
    auto unused = converted.imbue(std::locale{ std::wcout.getloc(), std::move(dec_sep) }); //locale takes owner ship of dec_sep
    converted << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
    
    {   //write csv header
        for (size_t i = 0; i < sliding_window_length; i++) {
            converted << i+1 << "; ";
        }
        converted << "Subclass; Exact" << std::endl;
    }


    if (!mLearning_Data.empty()) {       
        size_t last_recent_idx = mLearning_Data.size() - 1;

        for (size_t predicted_idx = mLearning_Data.size() - 1; predicted_idx > 0; predicted_idx--) {
            const double predicted_time = mLearning_Data[predicted_idx].device_time;

            //find the last value in the recent sequence for the given prediction
            while ((mLearning_Data[last_recent_idx].device_time + dt > predicted_time) && (last_recent_idx > 0))
                last_recent_idx--;

            //if ((last_recent_idx==0) && (mLearning_Data[last_recent_idx].device_time + dt > predicted_time))
             //  continue;   //no data available

            {//write the recent sequence
                size_t first_recent_idx = 0;

                const size_t recent_levels_count = last_recent_idx - first_recent_idx + 1;
                if (recent_levels_count < sliding_window_length) {
                    for (size_t i = 0; i < sliding_window_length - recent_levels_count; i++) {
                        converted << "n/a; ";
                    }
                }
                else
                    first_recent_idx = last_recent_idx - sliding_window_length;

                for (size_t i = first_recent_idx; i <= last_recent_idx; i++) {
                    converted << mLearning_Data[i].level << "; ";
                }
            }

            //write the subclassed level
            converted << pattern_prediction::Subclassed_Level(mLearning_Data[predicted_idx].level) << "; ";

            //write the exact level
            converted << mLearning_Data[predicted_idx].level << std::endl;
        }
    }


    return converted.str();
}

void CPattern_Prediction_Data::Encounter() {
    mEncountered = true;
}

bool CPattern_Prediction_Data::Was_Encountered() const {
    return mEncountered;
}

std::stringstream CPattern_Prediction_Data::Level_Series() const {
    std::stringstream result;


    bool fired_once = false;
    for (const auto e : mLearning_Data) {
        if (fired_once)
            result << " ";
        fired_once = true;

        result << dbl_2_str(e.level);
    }

    return result;
}