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

#include "cvga.h"

#include <iomanip>

#include <scgms/iface/UIIface.h>
#include <scgms/rtl/UILib.h>
#include <scgms/utils/drawing/IRenderer.h>
#include <scgms/utils/drawing/SVGRenderer.h>
#include <scgms/utils/string_utils.h>

#undef min
#undef max

NDrawing_Error CCVGA_View::Draw(std::string& target, const TDraw_Options_Local& opts, const IDrawing_Data_Source& source) {
	drawing::Drawing draw;

	std::vector<std::pair<double, double>> points;
	for (auto& segId : opts.segment_ids) {
		const auto& seg = source.Get_Segment(segId);
		
		const auto igItr = seg.mPlots_Signals.find(scgms::signal_IG);
		if (igItr == seg.mPlots_Signals.end()) {
			continue;
		}

		const auto& ig = igItr->second.mPlots_Values;
		if (ig.empty()) {
			continue;
		}

		double tmpMin = ig[0].value, tmpMax = ig[0].value;
		for (const auto& val : ig) {
			if (val.value < tmpMin) {
				tmpMin = val.value;
			}
			if (val.value > tmpMax) {
				tmpMax = val.value;
			}
		}

		points.push_back({ tmpMin, tmpMax });
	}

	auto& grp = draw.Root().Add<drawing::Group>("root");

	constexpr double marginTop = 5;
	constexpr double marginRight = 10;
	constexpr double marginBottom = 40;
	constexpr double marginLeft = 40;

	const double startX = marginLeft;
	const double startY = marginTop;
	const double endX = opts.width - marginRight;
	const double endY = opts.height - marginBottom;

	const double blockWidth = (endX - startX) / 3.0;
	const double blockHeight = (endY - startY) / 3.0;

	// background scope
	{
		auto& plotBg = grp.Add<drawing::Group>("background");

		// A (green)
		plotBg.Add<drawing::Rectangle>(startX, startY + 2.0 * blockHeight, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#00FF00"));

		plotBg.Add<drawing::Text>(startX + 0.5 * blockWidth, startY + 2.5 * blockHeight, "A")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#00AA00"));

		// upper, mid and lower B (dark green)
		plotBg.Add<drawing::Rectangle>(startX, startY + blockHeight, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#008000"));
		plotBg.Add<drawing::Rectangle>(startX + blockWidth, startY + blockHeight, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#008000"));
		plotBg.Add<drawing::Rectangle>(startX + blockWidth, startY + 2.0 * blockHeight, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#008000"));

		plotBg.Add<drawing::Text>(startX + 0.5 * blockWidth, startY + 1.5 * blockHeight, "Upper B")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#004000"));
		plotBg.Add<drawing::Text>(startX + 1.5 * blockWidth, startY + 1.5 * blockHeight, "B")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#004000"));
		plotBg.Add<drawing::Text>(startX + 1.5 * blockWidth, startY + 2.5 * blockHeight, "Lower B")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#004000"));

		// upper and lower C (yellow)
		plotBg.Add<drawing::Rectangle>(startX, startY, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#FFFF00"));
		plotBg.Add<drawing::Rectangle>(startX + 2.0 * blockWidth, startY + 2.0 * blockHeight, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#FFFF00"));

		plotBg.Add<drawing::Text>(startX + 0.5 * blockWidth, startY + 0.5 * blockHeight, "Upper C")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#808000"));
		plotBg.Add<drawing::Text>(startX + 2.5 * blockWidth, startY + 2.5 * blockHeight, "Lower C")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#808000"));

		// upper and lower D (orange)
		plotBg.Add<drawing::Rectangle>(startX + blockWidth, startY, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#FFA500"));
		plotBg.Add<drawing::Rectangle>(startX + 2.0 * blockWidth, startY + blockHeight, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#FFA500"));

		plotBg.Add<drawing::Text>(startX + 1.5 * blockWidth, startY + 0.5 * blockHeight, "Upper D")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#AA7000"));
		plotBg.Add<drawing::Text>(startX + 2.5 * blockWidth, startY + 1.5 * blockHeight, "Lower D")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#AA7000"));

		// E (red)
		plotBg.Add<drawing::Rectangle>(startX + 2.0 * blockWidth, startY, blockWidth, blockHeight).Set_Fill_Color(RGBColor::From_HTML_Color("#FF0000"));

		plotBg.Add<drawing::Text>(startX + 2.5 * blockWidth, startY + 0.5 * blockHeight, "E")
			.Set_Font_Size(36)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE)
			.Set_Font_Weight(drawing::Text::FontWeight::BOLD)
			.Set_Fill_Color(RGBColor::From_HTML_Color("#A00000"));
	}

	// axes scope
	{
		auto& axes = grp.Add<drawing::Group>("axes");

		axes.Set_Default_Stroke_Color(RGBColor::From_HTML_Color("#000000"));
		axes.Set_Default_Stroke_Width(2.0);

		axes.Add<drawing::Line>(startX, startY, startX, endY);
		axes.Add<drawing::Line>(startX, endY, endX, endY);

		axes.Add<drawing::Text>(startX, endY + 16, "&gt;6.1")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

		axes.Add<drawing::Text>(startX + blockWidth, endY + 16, "5.0")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

		axes.Add<drawing::Text>(startX + 2*blockWidth, endY + 16, "3.9")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

		axes.Add<drawing::Text>(endX - 6, endY + 16, "&lt;2.8")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);


		axes.Add<drawing::Text>(startX - 4, endY, "&lt;6.1")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::END);

		axes.Add<drawing::Text>(startX - 4, endY - blockHeight + 6, "10.0")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::END);

		axes.Add<drawing::Text>(startX - 4, endY - 2.0 * blockHeight + 6, "16.7")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::END);

		axes.Add<drawing::Text>(startX - 4, startY + 14, "&gt;22.2")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::END);

		axes.Add<drawing::Text>((endX - startX) / 2.0, endY + 26, "Minimum IG [mmol/l]")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::END);

		std::ostringstream rotate;
		rotate << "rotate(-90 " << startX - 14 << "," << (endY - startY) / 2.5 << ")";

		axes.Add<drawing::Text>(startX - 14, (endY - startY) / 2.5, "Maximum IG [mmol/l]")
			.Set_Font_Size(14)
			.Set_Anchor(drawing::Text::TextAnchor::END)
			.Set_Transform(rotate.str());
	}

	// points
	{
		auto& pts = grp.Add<drawing::Group>("points");

		pts.Set_Default_Stroke_Color(RGBColor::From_HTML_Color("#0000F0"));
		pts.Set_Default_Fill_Opacity(0.0);
		pts.Set_Default_Stroke_Width(1.5);

		auto igToX = [&startX, &endX](double ig) {
			const double norm = (ig - 2.8) / (6.1 - 2.8);
			if (norm < 0.0) {
				return endX;
			}
			else if (norm > 1.0) {
				return startX;
			}

			return endX - norm * (endX - startX);
		};

		auto igToY = [&startY, &endY](double ig) {
			const double norm = (ig - 6.1) / (22.2 - 6.1);
			if (norm < 0.0) {
				return endY;
			}
			else if (norm > 1.0) {
				return startY;
			}

			return endY - norm * (endY - startY);
		};

		for (const auto& pt : points) {
			pts.Add<drawing::Circle>(igToX(pt.first), igToY(pt.second), 4.0);
		}
	}

	// render prepared canvas to string
	CSVG_Renderer renderer(opts.width, opts.height, target);
	draw.Render(renderer);

	return NDrawing_Error::Ok;
}
