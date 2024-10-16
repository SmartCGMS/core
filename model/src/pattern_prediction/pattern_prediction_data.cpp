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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "pattern_prediction_data.h"

#include <scgms/utils/DebugHelper.h>
#include <scgms/utils/math_utils.h>
#include <scgms/utils/string_utils.h>

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
	if (Is_Any_NaN(level)) {
		return;
	}

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
	//this is a stub for the future, shall we ever find a reliable way for on-line learning
	//for now, we just repeat the recent prediction or the configured level - depending on the do_not_learn switch
	
	if (mInvalidated) {
		const size_t full_set_n = mFull ? mState.size() : mHead;
		if (full_set_n > 0) {
			const size_t last_write_position = mHead > 0 ? mHead - 1 : mState.size() - 1;
			mRecent_Prediction = mState[last_write_position];
			mInvalidated = false;
		}
		else {
			mRecent_Prediction = std::numeric_limits<double>::quiet_NaN();
		}
	}

	return mRecent_Prediction;
}
   
CPattern_Prediction_Data::operator bool() const {
	return (mHead > 0) || mFull;
}


void CPattern_Prediction_Data::Set_State(const double& level) {    
	for (auto& e : mState) {
		e = level;
	}
	
	mHead = 0;
	mFull = true;
	mRecent_Prediction = level;
	mInvalidated = false;
}

void CPattern_Prediction_Data::State_from_String(const std::wstring& state) {
	
	bool ok = false;
	const auto converted = str_2_dbls(state.c_str(), ok);
	if (ok) {
		for (const auto value : converted) {
			push(0.0, value);
		}
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
		if (not_empty) {
			converted << L" ";
		}
		else {
			not_empty = true;
		}

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
	
	//write csv header
	{
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
			while ((mLearning_Data[last_recent_idx].device_time + dt > predicted_time) && (last_recent_idx > 0)) {
				last_recent_idx--;
			}

			//if ((last_recent_idx==0) && (mLearning_Data[last_recent_idx].device_time + dt > predicted_time))
			//  continue;   //no data available

			//write the recent sequence
			{
				size_t first_recent_idx = 0;

				const size_t recent_levels_count = last_recent_idx - first_recent_idx + 1;
				if (recent_levels_count < sliding_window_length) {
					for (size_t i = 0; i < sliding_window_length - recent_levels_count; i++) {
						converted << "n/a; ";
					}
				}
				else {
					first_recent_idx = last_recent_idx - sliding_window_length;
				}

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
		if (fired_once) {
			result << " ";
		}
		fired_once = true;

		result << dbl_2_str(e.level);
	}

	return result;
}