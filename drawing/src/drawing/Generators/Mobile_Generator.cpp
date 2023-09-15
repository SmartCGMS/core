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

#include "Mobile_Generator.h"

// https://coolors.co/5390f5-eb5447-fbc21b-f48e00-46af62

const char* ColumnColor_Shade1 = "#A9C7FA";	// darkest
const char* ColumnColor_Shade2 = "#BAD2FB";
const char* ColumnColor_Shade3 = "#CBDDFC";
const char* ColumnColor_Shade4 = "#DCE8FD";
const char* ColumnColor_Shade5 = "#EDF3FE";	// lightest

const char* TextColor = "#244986";
const char* TargetRangeColor = "#6BBF81";
const char* TargetElevatedColor = "#FBC21B";

const char* MeasurementAboveColor = "#EB5447";
const char* MeasurementElevatedColor = "#FBC21B";
const char* MeasurementInRangeColor = "#46AF62";
const char* MeasurementBelowColor = "#EB5447";

const char* CarbsColumnColor = "#F48E00";
const char* CarbsCoBFillColor = "#F48E00";
const char* CarbsCoBStrokeColor = "#F48E00";

const char* InsulinBolusColumnColor = "#5390F5";
const char* InsulinBasalDotColor = "#5390F5";
const char* InsulinIoBFillColor = "#5390F5";
const char* InsulinIoBStrokeColor = "#5390F5";

std::pair<time_t, time_t> CMobile_Generator::Get_Display_Time_Range(const DataMap& inputData) const
{
	time_t maxVal = 0;

	for (const auto& mp : inputData)
	{
		for (const auto& val : mp.second.values)
		{
			if (val.date > maxVal)
				maxVal = val.date;
		}
	}

	if (maxVal < 0)
		maxVal = time(nullptr);

	// we align time to 3h blocks (to the future)

	maxVal /= ThreeHours;
	maxVal++;
	maxVal *= ThreeHours;

	// display last 12h
	// TODO: make this configurable after drawing modularization and refactoring
	return { maxVal - 12 * 60 * 60, maxVal };
}

std::string CMobile_Generator::Get_Time_Of_Day_Color(time_t curTime)
{
	const time_t curHr = (curTime % OneDay) / (60 * 60);
	if (curHr >= 21)		// 21-24
		return ColumnColor_Shade2;
	else if (curHr >= 18)	// 18-21
		return ColumnColor_Shade3;
	else if (curHr >= 15)	// 15-18
		return ColumnColor_Shade4;
	else if (curHr >= 12)	// 12-15
		return ColumnColor_Shade5;
	else if (curHr >= 9)	// 9-12
		return ColumnColor_Shade4;
	else if (curHr >= 6)	// 6-9
		return ColumnColor_Shade3;
	else if (curHr >= 3)	// 3-6
		return ColumnColor_Shade2;
	return ColumnColor_Shade1;
}

std::string CMobile_Generator::Get_Time_Of_Day_Title(time_t curTime)
{
	const time_t curHr = (curTime % OneDay) / (60 * 60);
	return std::to_string(curHr - curHr % 3) + ":00";
}