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
#include "Mobile_InsulinGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/Utility.h"

#include <algorithm>
#include <set>

int CMobile_Insulin_Generator::startX = 90;
int CMobile_Insulin_Generator::startY = 52;
int CMobile_Insulin_Generator::sizeX = 800;
int CMobile_Insulin_Generator::sizeY = 800;

constexpr time_t BolusColumnWidth = 10 * 60; // bolus columns is X minutes wide

void CMobile_Insulin_Generator::Set_Canvas_Size(int width, int height)
{
	sizeX = width;
	sizeY = height;
}

void CMobile_Insulin_Generator::Write_Description()
{
	auto& grp = mDraw.Root().Add<drawing::Group>("background");

	grp.Set_Default_Stroke_Width(0);
	grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(TextColor));

	time_t a;
	grp.Add<drawing::Text>(startX, 3.0 * MobileHeaderTextSize / 4.0, "TOTAL INSULIN (U)") // TODO: translate this
		.Set_Anchor(drawing::Text::TextAnchor::START)
		.Set_Font_Size(MobileHeaderTextSize);

	for (a = mTimeRange.first; a < mTimeRange.second; a += ThreeHours)
	{
		grp.Add<drawing::Rectangle>(startX + Normalize_Time_X(a), startY, Normalize_Time_X(a + ThreeHours) - Normalize_Time_X(a), sizeY)
			.Set_Fill_Color(RGBColor::From_HTML_Color(Get_Time_Of_Day_Color(a)));
	}

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(mMaxValueY*0.9) + MobileTextSize / 2, Utility::Format_Decimal(mMaxValueY, 1))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(std::round((mMaxValueY*0.9) / 2.0)) + MobileTextSize / 2, Utility::Format_Decimal(std::round(mMaxValueY / 2.0), 1))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(0) - 4, Utility::Format_Decimal(0, 1))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);
}

void CMobile_Insulin_Generator::Write_Body()
{
	ValueVector& basal = Utility::Get_Value_Vector_Ref(mInputData, "basal_insulin");
	ValueVector& bolus = Utility::Get_Value_Vector_Ref(mInputData, "insulin");
	ValueVector& iob = Utility::Get_Value_Vector_Ref(mInputData, "iob");

	time_t maxTime = 0;

	mMaxValueY = 2.0; // use 2 U as base maximum
	for (auto& val : basal)
	{
		if (val.date < mTimeRange.first || val.date > mTimeRange.second)
			continue;
		if (val.value > mMaxValueY)
			mMaxValueY = val.value;
		if (val.date > maxTime)
			maxTime = val.date;
	}

	for (auto& val : bolus)
	{
		if (val.date < mTimeRange.first || val.date > mTimeRange.second)
			continue;
		if (val.value > mMaxValueY)
			mMaxValueY = val.value;
		if (val.date > maxTime)
			maxTime = val.date;
	}

	for (auto& val : iob)
	{
		if (val.date < mTimeRange.first || val.date > mTimeRange.second)
			continue;
		if (val.value > mMaxValueY)
			mMaxValueY = val.value;
		if (val.date > maxTime)
			maxTime = val.date;
	}

	Write_Description();

	// add 10% padding
	mMaxValueY *= 1.1;

	mDraw.Root().Set_Default_Stroke_Width(0);

	// IoB group scope
	{
		auto& grp = mDraw.Root().Add<drawing::Group>("iob");

		auto& poly = grp.Add<drawing::Polygon>(startX + Normalize_Time_X(mTimeRange.first), Normalize_Y(0));
		poly.Set_Stroke_Color(RGBColor::From_HTML_Color(InsulinIoBStrokeColor));
		poly.Set_Fill_Color(RGBColor::From_HTML_Color(InsulinIoBFillColor));
		poly.Set_Stroke_Width(3);
		poly.Set_Stroke_Opacity(0.5);
		poly.Set_Fill_Opacity(0.5);

		double lastX = Normalize_Time_X(mTimeRange.first);
		bool first = true;

		for (size_t i = 0; i < iob.size(); i++)
		{
			Value& val = iob[i];

			if (val.date > mTimeRange.second || val.date < mTimeRange.first)
				continue;

			if (first)
			{
				first = false;
				poly.Add_Point(startX + Normalize_Time_X(val.date), Normalize_Y(0));
			}

			poly.Add_Point(startX + Normalize_Time_X(val.date), Normalize_Y(val.value));

			lastX = Normalize_Time_X(val.date);
		}

		poly.Add_Point(startX + lastX, Normalize_Y(0));
	}

	// bolus group scope
	{
		auto& grp = mDraw.Root().Add<drawing::Group>("bolus");

		grp.Set_Default_Stroke_Width(0);
		grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(InsulinBolusColumnColor));

		for (size_t i = 0; i < bolus.size(); i++)
		{
			Value& val = bolus[i];

			if (val.date > mTimeRange.second || val.date < mTimeRange.first)
				continue;

			const double ColumnWidth = Normalize_Time_X(val.date + BolusColumnWidth) - Normalize_Time_X(val.date);

			grp.Add<drawing::Rectangle>(startX + Normalize_Time_X(val.date) - ColumnWidth / 2.0, Normalize_Y(val.value), ColumnWidth, Normalize_Y(0) - Normalize_Y(val.value));
		}
	}

	// basal group scope
	{
		auto& grp = mDraw.Root().Add<drawing::Group>("basal");

		double lastValue = -1000;

		grp.Set_Default_Stroke_Width(0);
		grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(InsulinBasalDotColor));

		for (size_t i = 0; i < basal.size(); i++)
		{
			Value& val = basal[i];

			if (val.date > mTimeRange.second || val.date < mTimeRange.first)
				continue;

			// do not draw zero or near-zero boluses
			if (val.value < 0.0001)
				continue;

			// val.value is in U/hr, by determining the time span, we could determine how much basal insulin were delivered in U
			// TODO: change this when we have a pump implemented (either virtual or physical); the pump then should transform
			//       basal rate to actually delivered units (and that's what we intend to visualize)

			/*if (i < basal.size() - 1 && basal[i + 1].date < mTimeRange.second)
			{
				const time_t timeSpan = basal[i + 1].date - val.date;
				const double basalDelivered = (static_cast<double>(timeSpan)*val.value) / (60.0 * 60.0);

				grp.Add<drawing::Circle>(startX + Normalize_Time_X(val.date), Normalize_Y(basalDelivered), 6);
			}*/

			if (val.value != lastValue)
			{
				grp.Add<drawing::Circle>(startX + Normalize_Time_X(val.date), Normalize_Y(val.value), 4);
				lastValue = val.value;
			}
		}
	}
}

double CMobile_Insulin_Generator::Normalize_Time_X(time_t date) const
{
	return ((static_cast<double>(date - mTimeRange.first) / static_cast<double>(mTimeRange.second - mTimeRange.first)) * (sizeX - startX));
}

double CMobile_Insulin_Generator::Normalize_Y(double val) const
{
	double v = (sizeY - startY) * (val / mMaxValueY);
	// invert axis
	return startY + ((sizeY - startY) - v);
}

std::string CMobile_Insulin_Generator::Build_SVG()
{
	try
	{
		Write_Body();
	}
	catch (...)
	{
		//
	}

	std::string svg;
	CSVG_Renderer renderer(sizeX, sizeY, svg);
	mDraw.Render(renderer);

	return svg;
}

CMobile_Insulin_Generator::CMobile_Insulin_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
	: CMobile_Generator(inputData, maxValue, localization, mmolFlag)
{
	mTimeRange = Get_Display_Time_Range(inputData);
}
