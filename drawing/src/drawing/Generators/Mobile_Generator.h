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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <map>
#include <vector>

#include "IGenerator.h"
#include "../Containers/Value.h"

#include "../../../../../common/utils/drawing/SVGRenderer.h"

constexpr time_t OneDay = 24 * 60 * 60;
constexpr time_t ThreeHours = 3 * 60 * 60;

// TODO: make this configurable after drawing refacoring
constexpr double BG_Target_Min = 3.8;
constexpr double BG_Target_Max = 7.0;
constexpr double BG_Elevated_Max = 10.8;

constexpr int MobileTextSize = 38;
constexpr int MobileHeaderTextSize = 48;

extern const char* ColumnColor_Shade1;
extern const char* ColumnColor_Shade2;
extern const char* ColumnColor_Shade3;
extern const char* ColumnColor_Shade4;
extern const char* ColumnColor_Shade5;

extern const char* TextColor;
extern const char* TargetRangeColor;
extern const char* TargetElevatedColor;

extern const char* MeasurementAboveColor;
extern const char* MeasurementElevatedColor;
extern const char* MeasurementInRangeColor;
extern const char* MeasurementBelowColor;

extern const char* CarbsColumnColor;
extern const char* CarbsCoBFillColor;
extern const char* CarbsCoBStrokeColor;

extern const char* InsulinBolusColumnColor;
extern const char* InsulinBasalDotColor;
extern const char* InsulinIoBFillColor;
extern const char* InsulinIoBStrokeColor;

/*
 * Mobile generator parent
 */
class CMobile_Generator : public IGenerator
{
	protected:
		std::pair<time_t, time_t> Get_Display_Time_Range(const DataMap& inputData) const;
		static std::string Get_Time_Of_Day_Color(time_t curTime);
		static std::string Get_Time_Of_Day_Title(time_t curTime);

		drawing::Drawing mDraw;

	public:
		CMobile_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
			: IGenerator(inputData, maxValue, localization, mmolFlag)
		{
		}
};
