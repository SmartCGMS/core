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

#include "../general.h"
#include "Mobile_CarbsGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/Utility.h"

#include <algorithm>
#include <set>

int CMobile_Carbs_Generator::startX = 90;
int CMobile_Carbs_Generator::startY = 52;
int CMobile_Carbs_Generator::sizeX = 800;
int CMobile_Carbs_Generator::sizeY = 800;

constexpr time_t DosageColumnWidth = 10 * 60; // column is "X minutes wide"

void CMobile_Carbs_Generator::Set_Canvas_Size(int width, int height)
{
	sizeX = width;
	sizeY = height;
}

void CMobile_Carbs_Generator::Write_Description()
{
	auto& grp = mDraw.Root().Add<drawing::Group>("background");

	grp.Set_Default_Stroke_Width(0);
	grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(TextColor));

	time_t a;
	grp.Add<drawing::Text>(startX, 3.0 * MobileHeaderTextSize / 4.0, "TOTAL CARBOHYDRATES (g)") // TODO: translate this
		.Set_Anchor(drawing::Text::TextAnchor::START)
		.Set_Font_Size(MobileHeaderTextSize);

	for (a = mTimeRange.first; a < mTimeRange.second; a += ThreeHours)
	{
		grp.Add<drawing::Rectangle>(startX + Normalize_Time_X(a), startY, Normalize_Time_X(a + ThreeHours) - Normalize_Time_X(a), sizeY)
			.Set_Fill_Color(RGBColor::From_HTML_Color(Get_Time_Of_Day_Color(a)));
	}

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(mMaxValueY*0.9) + MobileTextSize / 2, Utility::Format_Decimal(mMaxValueY, 0))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(std::round((mMaxValueY*0.9) / 2.0)) + MobileTextSize / 2, Utility::Format_Decimal(std::round(mMaxValueY / 2.0), 0))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);

	grp.Add<drawing::Text>(startX - 8, Normalize_Y(0) - 4, Utility::Format_Decimal(0, 0))
		.Set_Anchor(drawing::Text::TextAnchor::END)
		.Set_Font_Size(MobileTextSize)
		.Set_Font_Weight(drawing::Text::FontWeight::BOLD);
}

void CMobile_Carbs_Generator::Write_Body()
{
	ValueVector& cob = Utility::Get_Value_Vector_Ref(mInputData, "cob");
	ValueVector& carbs = Utility::Get_Value_Vector_Ref(mInputData, "carbs");

	mMaxValueY = 1.0;

	for (auto& val : carbs)
	{
		if (val.date > mTimeRange.second || val.date < mTimeRange.first)
			continue;
		if (val.value > mMaxValueY)
			mMaxValueY = val.value;
	}

	for (auto& val : cob)
	{
		if (val.date < mTimeRange.first || val.date > mTimeRange.second)
			continue;
		if (val.value > mMaxValueY)
			mMaxValueY = val.value;
	}

	// insert 10% padding
	mMaxValueY *= 1.1;

	Write_Description();

	mDraw.Root().Set_Default_Stroke_Width(0);

	// CoB group scope
	{
		auto& grp = mDraw.Root().Add<drawing::Group>("cob");

		auto& poly = grp.Add<drawing::Polygon>(startX + Normalize_Time_X(mTimeRange.first), Normalize_Y(0));
		poly.Set_Stroke_Color(RGBColor::From_HTML_Color(CarbsCoBStrokeColor));
		poly.Set_Fill_Color(RGBColor::From_HTML_Color(CarbsCoBFillColor));
		poly.Set_Stroke_Width(3);
		poly.Set_Opacity(0.5);

		double lastX = Normalize_Time_X(mTimeRange.first);
		bool first = true;

		for (size_t i = 0; i < cob.size(); i++)
		{
			Value& val = cob[i];

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

	// carbohydrates group scope
	{
		auto& grp = mDraw.Root().Add<drawing::Group>("carbs");

		grp.Set_Default_Stroke_Width(0);
		grp.Set_Default_Fill_Color(RGBColor::From_HTML_Color(CarbsColumnColor));

		for (auto& val : carbs)
		{
			if (val.date > mTimeRange.second || val.date < mTimeRange.first)
				continue;

			const double ColWidth = Normalize_Time_X(val.date + DosageColumnWidth) - Normalize_Time_X(val.date);

			grp.Add<drawing::Rectangle>(startX + Normalize_Time_X(val.date) - ColWidth / 2.0, Normalize_Y(val.value), ColWidth, Normalize_Y(0) - Normalize_Y(val.value));
		}
	}
}

double CMobile_Carbs_Generator::Normalize_Time_X(time_t date) const
{
	return ((static_cast<double>(date - mTimeRange.first) / static_cast<double>(mTimeRange.second - mTimeRange.first)) * (sizeX - startX));
}

double CMobile_Carbs_Generator::Normalize_Y(double val) const
{
	double v = (sizeY - startY) * (val / mMaxValueY);
	// invert axis
	return startY + ((sizeY - startY) - v);
}

std::string CMobile_Carbs_Generator::Build_SVG()
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

CMobile_Carbs_Generator::CMobile_Carbs_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
	: CMobile_Generator(inputData, maxValue, localization, mmolFlag)
{
	mTimeRange = Get_Display_Time_Range(inputData);
}
