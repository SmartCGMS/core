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
#include "ClarkGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/PlotBackgroundMap.h"
#include "../Misc/Utility.h"

int CClark_Generator::startX = 100;
int CClark_Generator::maxX = 400;
int CClark_Generator::maxY = 400;
int CClark_Generator::sizeX = 600;
int CClark_Generator::sizeY = 600;

constexpr double Clark_Norm_Limit = 460.0;
constexpr double Clark_Max_MgDl = 400.0;

void CClark_Generator::Set_Canvas_Size(int width, int height) {
	// we need the image to be square
	if (width < height) {
		height = width;
	}
	else {
		width = height;
	}

	sizeX = width;
	sizeY = height;

	maxX = width;
	maxY = height;
}

void CClark_Generator::Write_Plot_Map() {
	// zone a
	mSvg.Set_Stroke(1, "grey", "none");

	// background map group scope
	{
		SVG::GroupGuard grp(mSvg, "graphMap", true);

		Write_Normalized_Path(Coords_a);
		mSvg.Draw_Text(Normalize_X(350), Normalize_Y(330), "A", "middle", "grey", 14);
		// zone b up
		Write_Normalized_Path(Coords_b_up);
		mSvg.Draw_Text(Normalize_X(220), Normalize_Y(300), "B", "middle", "grey", 14);
		// zone b down
		Write_Normalized_Path(Coords_b_down);
		mSvg.Draw_Text(Normalize_X(350), Normalize_Y(220), "B", "middle", "grey", 14);
		// zone c up
		Write_Normalized_Path(Coords_c_up);
		mSvg.Draw_Text(Normalize_X(120), Normalize_Y(290), "C", "middle", "grey", 14);
		// zone c down
		Write_Normalized_Path(Coords_c_down);
		mSvg.Draw_Text(Normalize_X(165), Normalize_Y(25), "C", "middle", "grey", 14);
		// zone d up
		Write_Normalized_Path(Coords_d_up);
		mSvg.Draw_Text(Normalize_X(35), Normalize_Y(125), "D", "middle", "grey", 14);
		// zone d down
		Write_Normalized_Path(Coords_d_down);
		mSvg.Draw_Text(Normalize_X(320), Normalize_Y(125), "D", "middle", "grey", 14);
		// zone e up
		Write_Normalized_Path(Coords_e_up);
		mSvg.Draw_Text(Normalize_X(35), Normalize_Y(290), "E", "middle", "grey", 14);
		// zone e down
		Write_Normalized_Path(Coords_e_down);
		mSvg.Draw_Text(Normalize_X(290), Normalize_Y(35), "E", "middle", "grey", 14);
	}

	mSvg.Set_Default_Stroke();
}

void CClark_Generator::Write_Descripton() {
	mSvg.Set_Stroke(1, "black", "none");

	// axis group scope
	{
		SVG::GroupGuard grp(mSvg, "axis", true);

		// y-axis
		std::ostringstream rotate;
		double yPom = maxY / 2.0;
		rotate << "rotate(-90 " << startX - 40 << "," << yPom << ")";

		std::stringstream str;
		str << tr("counted");
		str << (mMmolFlag ? " [mmol/l] " : " [mg/dl] ");
		mSvg.Draw_Text_Transform(startX - 40, yPom, str.str(), rotate.str(), "black", 14);
		mSvg.Line(startX, Normalize_Y(Clark_Max_MgDl), startX, Normalize_Y(0));

		// x-axis
		str.str("");
		str << tr("measured");
		str << (mMmolFlag ? " [mmol/l] " : " [mg/dl] ");
		mSvg.Draw_Text(startX + maxX / 2, Normalize_Y(0) + 50, str.str(), "middle", "black", 14);
		mSvg.Line(startX, Normalize_Y(0), Normalize_X(Clark_Max_MgDl), Normalize_Y(0));
	}

	// axis description group scope
	{
		SVG::GroupGuard grp(mSvg, "description", true);

		// TODO: rewrite Clark and Parkes generators to use mmol/l and get rid of this conversion mess

		constexpr int step = static_cast<int>(Utility::MmolL_To_MgDl(2.0));

		// x-axis description
		for (int i = 0; i <= Clark_Norm_Limit; i += step) {
			double x_d = Normalize_X(i);
			mSvg.Line(x_d, Normalize_Y(0), x_d, Normalize_Y(0) + 10);
			mSvg.Draw_Text(x_d, Normalize_Y(0) + 22, mMmolFlag ? Utility::Format_Decimal(round(Utility::MgDl_To_MmolL(i)), 0) : Utility::Format_Decimal(i, 0), "middle", "black", 12);
		}

		// y-axis description
		for (int i = 0; i <= Clark_Norm_Limit; i += step) {
			double y_d = Normalize_Y(i);
			mSvg.Line(startX - 10, y_d, startX, y_d);
			mSvg.Draw_Text(startX - 25, y_d, mMmolFlag ? Utility::Format_Decimal(round(Utility::MgDl_To_MmolL(i)), 0) : Utility::Format_Decimal(i, 0), "middle", "black", 12);
		}
	}

	mSvg.Set_Default_Stroke();
}

void CClark_Generator::Write_Legend_Item(std::string id, std::string text, std::string function, double y) {
	SVG::GroupGuard grp(mSvg, id, false);

	mSvg.Link_Text_color(startX, y, text, function, 12);
}

void CClark_Generator::Write_Body() {
	ValueVector istVector = Utility::Get_Value_Vector(mInputData, "ist");
	ValueVector bloodVector = Utility::Get_Value_Vector(mInputData, "blood");

	Write_Plot_Map();
	Write_Descripton();

	// axis
	mSvg.Set_Stroke(1, "red", "none");
	mSvg.Line_Dash(Normalize_X(0), Normalize_Y(0), Normalize_X(Clark_Max_MgDl), Normalize_Y(Clark_Max_MgDl));

	SVG istEl;

	std::map<std::string, SVG> calcElMap;

	size_t curColorIdx = 0;

	for (auto& dataVector : mInputData) {
		if (dataVector.second.calculated && !dataVector.second.empty) {
			calcElMap[dataVector.second.identifier].Set_Stroke(3, Utility::Curve_Colors[curColorIdx], Utility::Curve_Colors[curColorIdx]);
			calcElMap[dataVector.second.identifier].Group_Open(dataVector.second.identifier + "Curve", true);

			curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();
		}
	}

	istEl.Set_Stroke(3, "blue", "blue");
	istEl.Group_Open("istCurve", true);
	istEl.Set_Stroke_Visible(false);

	for (size_t i = 0; i < bloodVector.size(); i++) {
		Value& measuredBlood = bloodVector[i];
		Value searchValue;

		if (Utility::Find_Value(searchValue, measuredBlood, istVector)) {
			istEl.Point(Normalize_X(Clark_Norm_Limit * (Utility::MmolL_To_MgDl(measuredBlood.value) / Clark_Max_MgDl)), Normalize_Y(Clark_Norm_Limit * Utility::MmolL_To_MgDl(searchValue.value) / Clark_Max_MgDl), 3);
		}
	}

	for (auto& calcElPair : calcElMap) {
		auto& calcvector = mInputData[calcElPair.first].values;
		auto& refvector = mInputData[mInputData[calcElPair.first].refSignalIdentifier];

		Value searchValue;

		for (size_t i = 0; i < refvector.values.size(); i++) {
			if (Utility::Find_Value(searchValue, refvector.values[i], calcvector)) {
				calcElPair.second.Point(Normalize_X(Clark_Norm_Limit * Utility::MmolL_To_MgDl(refvector.values[i].value) / Clark_Max_MgDl), Normalize_Y(Clark_Norm_Limit * Utility::MmolL_To_MgDl(searchValue.value) / Clark_Max_MgDl), 3);
			}
		}
	}

	for (auto& calcElPair : calcElMap) {
		calcElPair.second.Group_Close();
	}

	istEl.Group_Close();
	mSvg << istEl.Dump();

	for (auto& calcElPair : calcElMap) {
		mSvg << calcElPair.second.Dump();
	}

	// legend group scope
	{
		SVG::GroupGuard grp(mSvg, "legend", false);

		double y = Normalize_Y(0) + 70;
		mSvg.Set_Stroke(1, "blue", "blue");
		Write_Legend_Item("ist", tr("ist"), "change_visibility_ist()", y);

		y += 20;

		curColorIdx = 0;

		for (auto& dataVector : mInputData) {
			if (dataVector.second.calculated && !dataVector.second.empty) {
				mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], Utility::Curve_Colors[curColorIdx]);
				Write_Legend_Item(dataVector.second.identifier, tr(dataVector.second.identifier), "", y); // TODO: proper "change visibility" function

				curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();

				y += 20;
			}
		}

		mSvg.Set_Default_Stroke();
	}
}

double CClark_Generator::Normalize_X(double date) const {
	return startX + (date / Clark_Norm_Limit)*maxX;
}

double CClark_Generator::Normalize_Y(double val) const {
	double v = (maxY * val * 0.95) / Clark_Norm_Limit;
	// invert axis
	return maxY - v - 0.15*sizeY;
}

std::string CClark_Generator::Build_SVG() {
	mSvg.Header(sizeX, sizeY);
	Write_Body();
	mSvg.Footer();

	return mSvg.Dump();
}

CClark_Generator::CClark_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
	: IGenerator(inputData, maxValue, localization, mmolFlag) {
	//
}
