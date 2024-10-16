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

#include "../general.h"
#include "Mobile_GlucoseGenerator.h"

#include "../Misc/Utility.h"

#include <algorithm>
#include <set>
#include <cmath>

int CMobile_Glucose_Generator::startX = 90;
int CMobile_Glucose_Generator::startY = MobileHeaderTextSize + 52; // header + times
int CMobile_Glucose_Generator::sizeX = 800;
int CMobile_Glucose_Generator::sizeY = 800;

void CMobile_Glucose_Generator::Set_Canvas_Size(int width, int height) {
	sizeX = width;
	sizeY = height;
}

void CMobile_Glucose_Generator::Write_Description() {
	auto& grp = mDraw.Root().Add<drawing::Group>("background");

	grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(TextColor));

	time_t a;
	grp.Add<drawing::Text>(startX, 3.0 * MobileHeaderTextSize / 4.0, "BLOOD GLUCOSE (mmol/L)")
		.Set_Anchor(drawing::Text::TextAnchor::START)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
		.Set_Font_Size(MobileHeaderTextSize);

	for (a = mTimeRange.first; a < mTimeRange.second; a += ThreeHours) {
		grp.Add<drawing::Rectangle>(startX + Normalize_Time_X(a), startY, Normalize_Time_X(a + ThreeHours) - Normalize_Time_X(a), sizeY)
			.Set_Fill_Color(RGBColor::From_HTML_Color(Get_Time_Of_Day_Color(a)));

		grp.Add<drawing::Text>(startX + Normalize_Time_X(a), 52 + 3.0 * 40 / 5.0, Get_Time_Of_Day_Title(a))
			.Set_Anchor((a == mTimeRange.first) ? drawing::Text::TextAnchor::START : drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Size(MobileTextSize)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD);
	}

	grp.Add<drawing::Text>(startX + Normalize_Time_X(a), 52 + 3.0 * 40 / 5.0, Get_Time_Of_Day_Title(a))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);

	grp.Set_Default_Stroke_Width(6);
	grp.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(TargetRangeColor));
	grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(TargetRangeColor));

	grp.Add<drawing::Line>(startX, Normalize_Y(BG_Target_Min), sizeX, Normalize_Y(BG_Target_Min))
		.Set_Stroke_Dash_Array({ 4, 12 });
	grp.Add<drawing::Line>(startX, Normalize_Y(BG_Target_Max), sizeX, Normalize_Y(BG_Target_Max))
		.Set_Stroke_Dash_Array({ 4, 12 });

	grp.Set_Default_Stroke_Width(6);
	grp.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(TargetElevatedColor));

	grp.Add<drawing::Line>(startX, Normalize_Y(BG_Elevated_Max), sizeX, Normalize_Y(BG_Elevated_Max))
		.Set_Stroke_Dash_Array({ 3, 16 });

	grp.Set_Default_Stroke_Width(0);
	grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(TextColor));
	grp.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(TextColor));

	const double targetMinY = Normalize_Y(BG_Target_Min) + 10;

	// draw minimum only if it wouldn't overlap target range texts
	if ((Normalize_Y(mMinValueY) - 4) - targetMinY > MobileTextSize) {
		grp.Add<drawing::Text>(startX - 8, Normalize_Y(mMinValueY) - 4, Utility::Format_Decimal(mMinValueY, 1))
			.Set_Anchor(drawing::Text::TextAnchor::END)
			.Set_Font_Size(MobileTextSize)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD);
	}

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(mMaxValueY) + 3.0*MobileTextSize / 4.0, Utility::Format_Decimal(mMaxValueY, 0))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);

	grp.Add<drawing::Text>(startX - 8, targetMinY, Utility::Format_Decimal(BG_Target_Min, 1))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(BG_Target_Max) + 10, Utility::Format_Decimal(BG_Target_Max, 1))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);
}

void CMobile_Glucose_Generator::Set_Stroke_By_Value(drawing::Group& target, double value, int width) {
	target.Set_Default_Stroke_Width(width);

	if (value > BG_Elevated_Max) {
		target.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(MeasurementAboveColor));
		target.Set_Default_Fill_Color(RGBColor::From_HTML_Color(MeasurementAboveColor));
	}
	else if (value > BG_Target_Max) {
		target.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(MeasurementElevatedColor));
		target.Set_Default_Fill_Color(RGBColor::From_HTML_Color(MeasurementElevatedColor));
	}
	else if (value < BG_Target_Min) {
		target.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(MeasurementBelowColor));
		target.Set_Default_Fill_Color(RGBColor::From_HTML_Color(MeasurementBelowColor));
	}
	else {
		target.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(MeasurementInRangeColor));
		target.Set_Default_Fill_Color(RGBColor::From_HTML_Color(MeasurementInRangeColor));
	}
}

static const std::array<std::string, 6> gPredictionMaps = {
	"{1FA03911-A62D-4ECF-AFE8-60B8379151B8}",	// 5min  (virtual 90)
	"{56C37AF2-FA68-43EF-88B6-8EA28E957824}",	// 10min (virtual 91)
	"{E959B878-74E4-479F-AECE-7AF2F1454498}",	// 15min (virtual 92)
	"{B6C0CAA3-01A3-402D-823D-57FF8F2D9C45}",	// 20min (virtual 93)
	"{173FFBB1-51BC-4E9B-9119-5B95EED29042}",	// 25min (virtual 94)
	"{BBE13567-C1F8-489D-8416-AB3565DE4F67}",	// 30min (virtual 95)
};

static const std::string gPhysicalActivitySignalName = "{F4438E9A-DD52-45BD-83CE-5E93615E62BD}";

static bool Fill_Pred_Vector(DataMap& inputMap, ValueVector& dst) {
	ValueVector tmp;

	time_t lastTime = 0;

	for (size_t i = 0; i < gPredictionMaps.size(); i++) {
		tmp = Utility::Get_Value_Vector(inputMap, gPredictionMaps[i]);
		if (tmp.empty()) {
			return false;
		}

		const auto& val = *tmp.rbegin();
		if (val.date < lastTime) {
			return false;
		}

		dst.push_back(val);
		lastTime = val.date;
	}

	return true;
}

void CMobile_Glucose_Generator::Write_Body() {
	ValueVector& istVector = Utility::Get_Value_Vector_Ref(mInputData, "ist");
	ValueVector predVector;// = Utility::Get_Value_Vector(mInputData, "{79EDF100-B0A2-4EE7-AB17-1637418DB15A}");

	if (!Fill_Pred_Vector(mInputData, predVector)) {
		predVector.clear();
	}

	// start at targetMin
	mMinValueY = BG_Target_Min;
	// start at 10 mmol/l
	mMaxValueY = 10.0;
	for (auto& val : istVector) {
		if (val.date < mTimeRange.first || val.date > mTimeRange.second) {
			continue;
		}
		if (val.value < 0 || val.value > 60.0) { // NOTE: temporary hack, will not be included in new drawing implementation
			continue;
		}
		if (val.value > mMaxValueY) {
			mMaxValueY = val.value;
		}
		if (val.value < mMinValueY) {
			mMinValueY = val.value;
		}
	}

	for (auto& val : predVector) {
		if (val.date < mTimeRange.first || val.date > mTimeRange.second) {
			continue;
		}
		if (val.value < 0 || val.value > 60.0) {// NOTE: temporary hack, will not be included in new drawing implementation
			continue;
		}
		if (val.value > mMaxValueY) {
			mMaxValueY = val.value;
		}
		if (val.value < mMinValueY) {
			mMinValueY = val.value;
		}
	}

	// add some bottom valuepadding
	mMinValueY *= 0.8;
	// add 10% padding
	mMaxValueY *= 1.1;

	Write_Description();

	double lastX = std::numeric_limits<double>::quiet_NaN(), lastY = 0, curX, curY;

	// ist group scope
	{
		auto& grp = mDraw.Root().Add<drawing::Group>("ist");

		for (auto& val : istVector) {
			if (val.date < mTimeRange.first || val.date > mTimeRange.second) {
				continue;
			}

			// NOTE: temporary hack, will not be included in new drawing implementation
			if (val.value < 0 || val.value > 60.0) {
				continue;
			}

			Set_Stroke_By_Value(grp, val.value);

			curX = startX + Normalize_Time_X(val.date);
			curY = Normalize_Y(val.value);

			Set_Stroke_By_Value(grp, val.value, 7);

			if (!std::isnan(lastX)) {
				grp.Add<drawing::Line>(lastX, lastY, curX, curY);
			}

			lastX = curX;
			lastY = curY;
		}
	}

	const double lastMeasuredX = lastX;
	bool foundInitial = false;

	// ist prediction group scope
	{
		auto& grp = mDraw.Root().Add<drawing::Group>("ist_pred");

		grp.Set_Fill_Opacity(0);

		drawing::PolyLine& polyline = grp.Add<drawing::PolyLine>(lastX, lastY);
		polyline.Set_Stroke_Color(RGBColor::From_HTML_Color(MeasurementElevatedColor));
		polyline.Set_Stroke_Width(7);
		polyline.Set_Fill_Opacity(0);
		polyline.Set_Stroke_Dash_Array({ 3.5, 3.5 });

		drawing::PolyLine& polyline2 = grp.Add<drawing::PolyLine>(lastX, lastY);
		polyline2.Set_Stroke_Color(RGBColor::From_HTML_Color(MeasurementInRangeColor));
		polyline2.Set_Stroke_Width(4);
		polyline2.Set_Fill_Opacity(0);
		polyline2.Set_Stroke_Dash_Array({ 2, 5 });

		for (auto& val : predVector) {
			if (val.date < mTimeRange.first || val.date > mTimeRange.second) {
				continue;
			}

			// NOTE: temporary hack, will not be included in new drawing implementation
			if (val.value < 0 || val.value > 60.0) {
				continue;
			}

			curX = startX + Normalize_Time_X(val.date);
			curY = Normalize_Y(val.value);

			if (!std::isnan(lastX) && curX > lastMeasuredX) {
				if (!foundInitial) {
					foundInitial = true;
					polyline.Set_Position(lastX, lastY);
					polyline.Add_Point(lastX, lastY);
					polyline2.Set_Position(lastX, lastY);
					polyline2.Add_Point(lastX, lastY);
				}
				polyline.Add_Point(curX, curY);
				polyline2.Add_Point(curX, curY);
			}

			lastX = curX;
			lastY = curY;
		}
	}

	// exercise group scope
	try {
		ValueVector& exerciseData = Utility::Get_Value_Vector_Ref(mInputData, gPhysicalActivitySignalName);

		auto& grp = mDraw.Root().Add<drawing::Group>("exercise");

		double lastExerciseValue = 0.0;
		time_t lastExerciseTime = 0;

		for (auto& val : exerciseData) {
			if (val.date < mTimeRange.first || val.date > mTimeRange.second) {
				lastExerciseValue = val.value;
				lastExerciseTime = val.date;
				continue;
			}

			grp.Set_Default_Stroke_Width(2);
			grp.Set_Default_Stroke_Color(RGBColor::From_HTML_Color(InsulinBasalDotColor));
			grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(InsulinBolusColumnColor));

			grp.Add<drawing::Line>(startX + Normalize_Time_X(lastExerciseTime < mTimeRange.first ? mTimeRange.first : lastExerciseTime), Normalize_Y(lastExerciseValue + mMinValueY),
				startX + Normalize_Time_X(val.date), Normalize_Y(lastExerciseValue + mMinValueY));

			grp.Add<drawing::Line>(startX + Normalize_Time_X(val.date), Normalize_Y(lastExerciseValue + mMinValueY),
				startX + Normalize_Time_X(val.date), Normalize_Y(val.value + mMinValueY));

			grp.Add<drawing::Circle>(startX + Normalize_Time_X(val.date), Normalize_Y(lastExerciseValue + mMinValueY), 2);
			grp.Add<drawing::Circle>(startX + Normalize_Time_X(val.date), Normalize_Y(val.value + mMinValueY), 2);

			lastExerciseValue = val.value;
			lastExerciseTime = val.date;
		}

		grp.Add<drawing::Line>(startX + Normalize_Time_X(lastExerciseTime < mTimeRange.first ? mTimeRange.first : lastExerciseTime), Normalize_Y(lastExerciseValue + mMinValueY),
			lastMeasuredX, Normalize_Y(lastExerciseValue + mMinValueY));

	}
	catch (...) {
		// no exercise data - fine, we don't need them
	}

	// blood calibration group scope
	try {
		ValueVector& bloodCalibrationVector = Utility::Get_Value_Vector_Ref(mInputData, "bloodCalibration");

		auto& grp = mDraw.Root().Add<drawing::Group>("bloodCal");

		for (auto& val : bloodCalibrationVector) {
			if (val.date < mTimeRange.first || val.date > mTimeRange.second) {
				continue;
			}

			Set_Stroke_By_Value(grp, val.value);

			grp.Add<drawing::Circle>(startX + Normalize_Time_X(val.date), Normalize_Y(val.value), 8);
		}
	}
	catch (...) {
		// when there's no blood calibration data, it's still ok
	}
}

double CMobile_Glucose_Generator::Normalize_Time_X(time_t date) const {
	return ((static_cast<double>(date - mTimeRange.first) / static_cast<double>(mTimeRange.second - mTimeRange.first)) * (sizeX - startX));
}

double CMobile_Glucose_Generator::Normalize_Y(double val) const {
	double v = (sizeY - startY) * ((val - mMinValueY) / (mMaxValueY - mMinValueY));
	// invert axis
	return startY + ((sizeY - startY) - v);
}

std::string CMobile_Glucose_Generator::Build_SVG() {
	mMaxValueY = (mMaxValue > 12) ? mMaxValue : 12;

	try {
		Write_Body();
	}
	catch (...) {
		//
	}

	std::string svg;
	CSVG_Renderer renderer(sizeX, sizeY, svg);
	mDraw.Render(renderer);

	return svg;
}

CMobile_Glucose_Generator::CMobile_Glucose_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
	: CMobile_Generator(inputData, maxValue, localization, mmolFlag) {
	mTimeRange = Get_Display_Time_Range(inputData);
}
