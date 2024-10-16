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
#include "AGPGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/Utility.h"
#include "../Containers/Day.h"

int CAGP_Generator::startX = 70;
int CAGP_Generator::maxX = 700;
int CAGP_Generator::maxY = 550;
int CAGP_Generator::sizeX = 800;
int CAGP_Generator::sizeY = 750;

constexpr double AGP_Max_Mgdl = Utility::MmolL_To_MgDl(32.0) + 1.0;

void CAGP_Generator::Set_Canvas_Size(int width, int height) {
	sizeX = width;
	sizeY = height;

	maxX = width;
	maxY = height;
}

static void Set_Hour_Min(time_t minD, time_t &start_date, int hour, int min) {
#ifdef _WIN32
	tm tim;
	_localtime(&tim, minD);
	tm* start_time_tm = &tim;
#else
	tm* start_time_tm = nullptr;
	_localtime(start_time_tm, minD);
#endif

	start_time_tm->tm_hour = hour;
	start_time_tm->tm_min = min;
	start_date = mktime(start_time_tm);
}

void CAGP_Generator::Write_Description() {
	mSvg.Set_Default_Stroke();

	// description group scope
	{
		SVG::GroupGuard grp(mSvg, "description", true);

		std::ostringstream rotate;
		double yPom = maxY / 2;
		rotate << "rotate(-90 " << startX - 50 << "," << yPom << ")";

		std::stringstream str;
		str << tr("axis_y");
		str << (mMmolFlag ? " [mmol/l] " : " [mg/dl] ");

		mSvg.Draw_Text_Transform(startX - 50, yPom, str.str(), rotate.str(), "black", 12);
		mSvg.Line(startX, Normalize_Y(0), startX, Normalize_Y(AGP_Max_Mgdl));
		// x-axis
		mSvg.Draw_Text(sizeX / 2, Normalize_Y(0) + 60, tr("axis_x"), "middle", "black", 12);
		mSvg.Line(startX, Normalize_Y(0), Normalize_X(24, 0), Normalize_Y(0));

		// x-axis description
		for (unsigned int i = 0; i <= 24; i += 1) {
			double x_d = Normalize_X(i, 0);
			mSvg.Line(x_d, Normalize_Y(0), x_d, Normalize_Y(0) + 10);
			mSvg.Draw_Text(x_d, Normalize_Y(0) + 30, std::to_string(i), "middle", "black", 12);
		}

		// y-axis description
		{
			double i = 0.0;
			do {
				double y_d = Normalize_Y(floor(i));
				mSvg.Line(startX - 10, y_d, startX, y_d);
				mSvg.Draw_Text(startX - 25, y_d, mMmolFlag ? Utility::Format_Decimal(round(Utility::MgDl_To_MmolL(i)), 3) : std::to_string(i), "middle", "black", 12);
				mSvg.Draw_Text(startX - 25, y_d, mMmolFlag ? Utility::Format_Decimal(round(Utility::MgDl_To_MmolL(i)), 3) : std::to_string(i), "middle", "black", 12);

				i += Utility::MmolL_To_MgDl(2.0);
			} while (i <= AGP_Max_Mgdl);
		}
	}

	mSvg.Set_Default_Stroke();
}

void CAGP_Generator::Write_Legend(Data istData, Data bloodData, Data bloodCalibrationData) {
	mSvg.Set_Default_Stroke();

	// legend group scope
	{
		SVG::GroupGuard grp(mSvg, "legend", true);

		double y = 40;

		if (istData.Is_Visible_Legend()) {
			std::string color = istData.visible ? "blue" : "grey";
			mSvg.Set_Stroke(1, color, color);

			SVG::GroupGuard istGrp(mSvg, "ist", true);
			mSvg.Link_Text_color(startX + 10, y, tr("ist"), "agp_change_visibility_ist()");

			y += 20;
		}

		size_t curColorIdx = 0;

		for (auto& dataVector : mInputData) {
			if (dataVector.second.calculated) {
				std::string color = dataVector.second.visible ? Utility::Curve_Colors[curColorIdx] : "grey";
				mSvg.Set_Stroke(1, color, color);

				SVG::GroupGuard diff2Grp(mSvg, dataVector.second.identifier, true);
				mSvg.Link_Text_color(startX + 10, y, tr(dataVector.second.identifier), ""); // TODO: proper "change visibility" function

				curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();

				y += 20;
			}
		}

		if (bloodData.Is_Visible_Legend()) {
			std::string color = bloodData.visible ? "red" : "grey";
			mSvg.Set_Stroke(1, color, color);

			SVG::GroupGuard bloodGrp(mSvg, "blood", true);
			mSvg.Link_Text_color(startX + 10, y, tr("blood"), "change_visibility_blood()");

			y += 20;
		}

		if (bloodCalibrationData.Is_Visible_Legend()) {
			std::string color = bloodCalibrationData.visible ? "#EC80FF" : "grey";
			mSvg.Set_Stroke(1, color, color);

			SVG::GroupGuard bloodGrp(mSvg, "bloodCalibration", true);
			mSvg.Link_Text_color(startX + 10, y, tr("bloodCalibration"), "change_visibility_bloodCalibration()");

			//y += 20; - no more used
		}

		mSvg.Set_Default_Stroke();
	}
}

void CAGP_Generator::Write_QuadraticBezirCurve_1(std::vector<TCoordinate> &values, std::vector<TCoordinate> &back) {

	if (values.empty() || back.empty()) {
		return;
	}

	size_t i;

	TCoordinate& start = values[0];
	mSvg << "<path d =\"M " << Normalize_X(0,0) << " " << Normalize_Y(0) << " L " << start.x << " " << start.y;

	for (i = 0; i < values.size() - 1; i += 2) {
		TCoordinate& control = values[i];
		TCoordinate& end = values[i + 1];

		mSvg << " Q " << control.x << " " << control.y << " " << end.x << " " << end.y;
	}

	TCoordinate& end = *(values.end() - 1);
	TCoordinate& end2 = *(back.end() - 1);

	mSvg << " L " << Normalize_X(24, 0) << " " << end.y << " L " << Normalize_X(24, 0) << " " << Normalize_Y(0.0);

	TCoordinate& start2 = back[0];
	mSvg << " M " << Normalize_X(0, 0) << " " << Normalize_Y(AGP_Max_Mgdl) << " L " << start2.x << " "<< start2.y;

	for (i = 0; i < back.size() - 1; i += 2) {
		TCoordinate& control = back[i];
		TCoordinate& end = back[i + 1];

		mSvg << " Q " << control.x << " " << control.y << " " << end.x << " " << end.y;
	}

	mSvg << " L " << Normalize_X(24, 0) << " " << end2.y << " L " << Normalize_X(24, 0) << " " << Normalize_Y(AGP_Max_Mgdl);
	mSvg << "\"/>" << std::endl;
}

void CAGP_Generator::Write_QuadraticBezirCurve_2(std::vector<TCoordinate> &values) {
	if (values.empty()) {
		return;
	}

	size_t i;

	TCoordinate& start = values[0];
	mSvg << "<path d=\"M " << Normalize_X(0, 0) << " " << start.y << " L " << start.x << " " << start.y;

	for (i = 0; i < values.size() - 1; i += 2) {
		TCoordinate& control = values[i];
		TCoordinate& end = values[i + 1];

		mSvg << " Q " << control.x << " " << control.y << " " << end.x << " " << end.y;
	}

	TCoordinate& end = *(values.end() - 1);
	mSvg << " L " << Normalize_X(24, 0) << " " << end.y;
	mSvg << "\"/>" << std::endl;
}

void CAGP_Generator::Write_Content(Day day, Data bloodData, Data bloodCalibrationData) {
	if (!day.Is_Empty()) {
		std::vector<TCoordinate> q0, q1, q2, q3, q4;
		day.Quartile(q0, q1, q2, q3, q4);

		const double x_rect = Normalize_X(0, 0);
		const double y_rect = Normalize_Y(AGP_Max_Mgdl);
		const double width_rect = Normalize_X(24, 0) - x_rect;
		const double height_rect = Normalize_Y(0) - y_rect;
		const double opacity = 1.0 / 2.0;

		// rectangle group scope
		{
			mSvg.Set_Stroke(1, "blue", "blue", 1 - opacity, 1 - opacity);
			SVG::GroupGuard grp(mSvg, "rect", true);
			mSvg.Rectangle(x_rect, y_rect, width_rect, height_rect);
		}

		// quartile group scope
		{
			mSvg.Set_Stroke(1, "white", "white", opacity, opacity);
			SVG::GroupGuard grp(mSvg, "1,3qv", true);
			Write_QuadraticBezirCurve_1(q1, q3);
		}

		// quartile group scope
		{
			mSvg.Set_Stroke(1, "white", "white", 1, 1);
			SVG::GroupGuard grp(mSvg, "0,4kv", true);
			Write_QuadraticBezirCurve_1(q0, q4);
		}

		// median group scope
		{
			mSvg.Set_Stroke(1, "blue", "none", -1, 1);
			SVG::GroupGuard grp(mSvg, "median", true);
			Write_QuadraticBezirCurve_2(q2);
		}

		// median group scope
		{
			mSvg.Set_Stroke(1, "white", "white", 1, 1);
			SVG::GroupGuard grp(mSvg, "whitening", true);
			mSvg.Line(Normalize_X(0, 0), Normalize_Y(AGP_Max_Mgdl), Normalize_X(24, 0), Normalize_Y(AGP_Max_Mgdl));
			mSvg.Line(Normalize_X(24, 0), Normalize_Y(AGP_Max_Mgdl), Normalize_X(24, 0), Normalize_Y(0));
		}
	}

	mSvg.Set_Default_Stroke();

	if (bloodData.Is_Visible()) {
		mSvg.Set_Stroke(3, "red", "red");

		SVG::GroupGuard grp(mSvg, "bloodCurve", true);
		for (size_t i = 0; i < bloodData.values.size(); i++) {
			Value& val = bloodData.values[i];

			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(Utility::MmolL_To_MgDl(val.value)), 3);
		}
	}

	if (bloodCalibrationData.Is_Visible()) {
		mSvg.Set_Stroke(3, "#EC80FF", "#EC80FF");

		SVG::GroupGuard grp(mSvg, "bloodCalibrationCurve", true);
		for (size_t i = 0; i < bloodCalibrationData.values.size(); i++) {
			Value& val = bloodCalibrationData.values[i];

			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(Utility::MmolL_To_MgDl(val.value)), 3);
		}
	}

	mSvg.Set_Default_Stroke();
}

Day CAGP_Generator::Aggregate_X_Axis(Data istData, Data bloodData) {
	Day d;

	bool calcVisible = false;
	for (auto& dataVector : mInputData) {
		if (dataVector.second.calculated && dataVector.second.Is_Visible()) {
			calcVisible = true;
			break;
		}
	}

	if (!istData.Is_Visible() && !calcVisible) {
		d.Set_Empty(true);
		return d;
	}

	time_t minD, maxD,start_date,end_date;
	Utility::Get_Boundary_Dates(istData.values, bloodData.values, minD, maxD);
	Set_Hour_Min(minD, start_date, 0, 0);
	Set_Hour_Min(maxD, end_date, 23, 59);

	const int time_min = 0;
	const int one_day_min = 60 * 24;
	const int step = 5;

	for (int minutes = time_min; minutes < one_day_min; minutes += step) {
		time_t actual_time = start_date + (minutes * 60);

		int day = 0;
		std::vector<double> points;
		while (true) {
			Value search_date;
			Value searchValue;
			search_date.date = actual_time + (day*SECONDS_IN_DAY);
			day++;

			if (search_date.date < minD) {
				continue;
			}

			if (search_date.date > maxD) {
				break;
			}

			if (istData.Is_Visible() && Utility::Find_Value(searchValue, search_date, istData.values)) {
				points.push_back(Normalize_Y(Utility::MmolL_To_MgDl(searchValue.value)));
			}

			for (auto& dataVector : mInputData) {
				if (dataVector.second.calculated && !dataVector.second.empty && Utility::Find_Value(searchValue, search_date, dataVector.second.values)) {
					points.push_back(Normalize_Y(Utility::MmolL_To_MgDl(searchValue.value)));
				}
			}
		}

		d.Insert(Normalize_Time_X(actual_time), points);
	}

	return d;
}

void CAGP_Generator::Write_Body() {
	Data istData = Utility::Get_Data_Item(mInputData, "ist");
	Data bloodData = Utility::Get_Data_Item(mInputData, "blood");
	Data bloodCalibrationData = Utility::Get_Data_Item(mInputData, "bloodCalibration");

	Day day = Aggregate_X_Axis(istData, bloodData);
	Write_Content(day, bloodData, bloodCalibrationData);
	Write_Description();
	Write_Legend(istData, bloodData, bloodCalibrationData);
}

double CAGP_Generator::Normalize_X(int hour, int minute) const {
	const double perHour = (double)(maxX - startX - 20) / 24.0;
	const double perMin = perHour / 60.0;
	return (double)startX + (hour*perHour) + (minute*perMin);
}

double CAGP_Generator::Normalize_Time_X(time_t x) const {
#ifdef _WIN32
	tm tim;
	_localtime(&tim, x);
	tm* timeinfo = &tim;
#else
	tm* timeinfo = nullptr;
	_localtime(timeinfo, x);
#endif

	return Normalize_X(timeinfo->tm_hour, timeinfo->tm_min);
}

double CAGP_Generator::Normalize_Y(double y) const {
	double v = maxY * (y / AGP_Max_Mgdl) * 0.9;
	// invert axis
	return (maxY - v) - 0.08*maxY;
}

std::string CAGP_Generator::Build_SVG() {
	mSvg.Header(sizeX, sizeY);
	Write_Body();
	mSvg.Footer();

	return mSvg.Dump();
}

CAGP_Generator::CAGP_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
	: IGenerator(inputData, maxValue, localization, mmolFlag) {
	//
}
