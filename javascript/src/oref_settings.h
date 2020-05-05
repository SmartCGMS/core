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

#include <string>
#include <vector>
#include <ctime>

constexpr double Rat_Seconds(double secs)
{
	return (secs / (24.0*60.0*60.0));
}

namespace oref_model
{
	constexpr double delta_history_start = Rat_Seconds(420);				// 17.5 min
	constexpr double delta_history_end = Rat_Seconds(150);					// 2.5 min
	constexpr double delta_history_sample_step = Rat_Seconds(30);

	constexpr double short_avgdelta_history_start = Rat_Seconds(1050);		// 17.5 min
	constexpr double short_avgdelta_history_end = Rat_Seconds(150);			// 2.5 min
	constexpr double short_avgdelta_history_sample_step = Rat_Seconds(60);

	constexpr double long_avgdelta_history_start = Rat_Seconds(2550);		// 42.5 min
	constexpr double long_avgdelta_history_end = Rat_Seconds(1050);			// 17.5 min
	constexpr double long_avgdelta_history_sample_step = Rat_Seconds(120);
}

struct COref_Instance_Data
{
	time_t mLastCarbTime = 0;
	double mLastCarbValue = 0;
	time_t mLastBolusTime = 0;
	double mLastBolusValue = 0;
	double mCurCOB = 0;
	time_t mCurTime = 0;
	time_t mLastTime = 0;
	double mShortAvgDelta, mLongAvgDelta, mDelta, mGlucose;
	std::vector<double> mCurIOB, mCurInsulinActivity;

	double mResultRate;
	double mResultDuration;
};

namespace oref_model
{
	struct TParameters;
}

namespace oref_utils
{
	void Substr_Replace(std::string& str, const std::string& what, const std::string& replace);
	std::string Prepare_Base_Program(const COref_Instance_Data& data, const oref_model::TParameters& parameters);
}
