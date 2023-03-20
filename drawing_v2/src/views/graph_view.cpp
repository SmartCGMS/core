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

#include "graph_view.h"

#include <iomanip>
#include <unordered_set>
#include <cmath>

#include "../../../../common/iface/UIIface.h"
#include "../../../../common/rtl/UILib.h"
#include "../../../../common/utils/drawing/IRenderer.h"
#include "../../../../common/utils/drawing/SVGRenderer.h"
#include "../../../../common/utils/string_utils.h"

#undef min
#undef max

NDrawing_Error CGraph_View::Draw(std::string& target, const TDraw_Options_Local& opts, const IDrawing_Data_Source& source)
{
	// NOTE: this is just a proof-of-the-concept implementation of one type of plots; this will surely be a subject of refactoring

	drawing::Drawing draw;

	auto& grp = draw.Root().Add<drawing::Group>("root");
	auto mCanvas_WidthOff = std::min(static_cast<double>(opts.width) * (1.0 / 10.0), 100.0);
	auto mCanvas_HeightOff = static_cast<double>(opts.height) - std::min(static_cast<double>(opts.height) * (1.0 / 10.0), 100.0);

	// axes
	grp.Add<drawing::Line>(mCanvas_WidthOff, mCanvas_HeightOff, opts.width, mCanvas_HeightOff)
		.Set_Stroke_Width(2);
	grp.Add<drawing::Line>(mCanvas_WidthOff, mCanvas_HeightOff, mCanvas_WidthOff, 0)
		.Set_Stroke_Width(2);

	// find minimum and maximum time, and maximum and minimum Y value
	double min_x = std::numeric_limits<double>::max();
	double max_x = -std::numeric_limits<double>::max();
	double min_y = 0.0;// std::numeric_limits<double>::max(); - not debugged yet
	double max_y = -std::numeric_limits<double>::max();

	for (const auto seg_id : opts.segment_ids) {
		const TPlot_Segment& segment = source.Get_Segment(seg_id);

		for (const auto& signal : segment.mPlots_Signals) {
			if (opts.signal_ids.find(signal.first) == opts.signal_ids.end())
				continue;

			scgms::TSignal_Descriptor desc = scgms::Null_Signal_Descriptor;

			if (scgms::get_signal_descriptor_by_id(signal.second.signal_id, desc))
			{
				for (const auto& val : signal.second.mPlots_Values)
				{
					min_y = std::min(min_y, val.value * desc.value_scale);
					max_y = std::max(max_y, val.value * desc.value_scale);
					min_x = std::min(min_x, val.device_time);
					max_x = std::max(max_x, val.device_time);
				}
			}
		}
	}

	if (max_x < min_y || max_y < min_y)
		return NDrawing_Error::Not_Enough_Values;

	max_y += 2.0; // always move the maximum value, so there's still room for descriptions, etc.

	size_t total_x_label_cnt = static_cast<size_t>( (static_cast<double>(opts.width) - mCanvas_WidthOff) / 150.0 );
	total_x_label_cnt = std::min(std::max(total_x_label_cnt, static_cast<size_t>(4)), static_cast<size_t>(18));

	// TODO: calculate steps, so the displayed values are with "nice" stepping (such as 1hr, 30min, 6hr, ...) and not necessarily evenly distributed
	double x_label_step_val = (max_x - min_x) / static_cast<double>(total_x_label_cnt);

	double x_label_step_px = (opts.width - mCanvas_WidthOff) * x_label_step_val / (max_x - min_x);

	std::string lastDate = "";

	// descriptions on X axis
	for (size_t i = 0; i < total_x_label_cnt; i++)
	{
		grp.Add<drawing::Text>(
				mCanvas_WidthOff + static_cast<double>(i) * x_label_step_px,
				mCanvas_HeightOff + 20,
				Rat_Time_To_Local_Time_Str(min_x + static_cast<double>(i) * x_label_step_val, "%H:%M")
			)
			.Set_Font_Size(12) // TODO: font scaling proportionally to drawing size
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

		std::string curDate = Rat_Time_To_Local_Time_Str(min_x + static_cast<double>(i) * x_label_step_val, "%d.%m.%Y");
		if (curDate != lastDate)
		{
			lastDate = curDate;

			grp.Add<drawing::Text>(
					mCanvas_WidthOff + static_cast<double>(i) * x_label_step_px,
					mCanvas_HeightOff + 34,
					curDate
				)
				.Set_Font_Size(12) // TODO: font scaling proportionally to drawing size
				.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);
		}
	}

	const double firstFullDay = std::floor(min_x + 1.0);
	const double firstFullHour = std::floor(min_x / scgms::One_Hour) * scgms::One_Hour;

	{
		double fdx = firstFullHour;
		do {
			double xpos = mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) * ((max_x - fdx) / (max_x - min_x));

			grp.Add<drawing::Line>(xpos, mCanvas_HeightOff, xpos, 0)
				.Set_Stroke_Color(RGBColor::From_HTML_Color("#F8F8F8"))
				.Set_Stroke_Opacity(1.0)
				.Set_Stroke_Width(1.0);

			fdx += scgms::One_Hour;
		} while (fdx < max_x);
	}

	{
		double fdx = firstFullDay;
		do {
			double xpos = mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) * ((max_x - fdx) / (max_x - min_x));

			grp.Add<drawing::Line>(xpos, mCanvas_HeightOff, xpos, 0)
				.Set_Stroke_Color(RGBColor::From_HTML_Color("#D8D8D8"))
				.Set_Stroke_Opacity(1.0)
				.Set_Stroke_Width(1.0);

			fdx += 1.0;
		} while (fdx < max_x);
	}

	grp.Add<drawing::Text>(mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) / 2, mCanvas_HeightOff + 50, "Time [day.month.year hour:minute]")
		.Set_Font_Size(15)
		.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

	const double y_step = (max_y - min_y < 10.0) ? 1.0 : 2.0;

	// descriptions on Y axis
	{
		
		double yf = y_step;
		do {
			grp.Add<drawing::Text>(
				mCanvas_WidthOff - 20,
				mCanvas_HeightOff - (yf / (max_y - min_y)) * mCanvas_HeightOff,
				utility::Format_Decimal(yf, 1)
				)
				.Set_Font_Size(12) // TODO: font scaling proportionally to drawing size
				.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

			grp.Add<drawing::Line>(
				mCanvas_WidthOff, mCanvas_HeightOff - (yf / (max_y - min_y)) * mCanvas_HeightOff,
				opts.width, mCanvas_HeightOff - (yf / (max_y - min_y)) * mCanvas_HeightOff
				)
				.Set_Stroke_Color(RGBColor::From_HTML_Color("#D8D8D8"))
				.Set_Stroke_Width(1);

			yf += y_step;
		} while (yf <= max_y);
	}

	std::unordered_set<GUID> usedSignals;

	double descriptionY = 20;
	for (const auto seg_id : opts.segment_ids) {

		const TPlot_Segment& segment = source.Get_Segment(seg_id);

		for (const auto& signal : segment.mPlots_Signals) {

			if (opts.signal_ids.find(signal.first) == opts.signal_ids.end())
				continue;

			scgms::TSignal_Descriptor desc = scgms::Null_Signal_Descriptor;

			if (scgms::get_signal_descriptor_by_id(signal.second.signal_id, desc))
			{
				// draw description text only on first appearance
				if (usedSignals.find(desc.id) == usedSignals.end()) {
					grp.Add<drawing::Text>(mCanvas_WidthOff + 20, descriptionY, Narrow_WChar(desc.signal_description) + " [" + Narrow_WChar(desc.unit_description) + "]")
						.Set_Font_Size(10)
						.Set_Anchor(drawing::Text::TextAnchor::START)
						.Set_Fill_Color(RGBColor::From_UInt32(desc.fill_color, true));

					usedSignals.insert(desc.id);
				}

				descriptionY += 20;

				//
				if (desc.visualization == scgms::NSignal_Visualization::smooth ||
					desc.visualization == scgms::NSignal_Visualization::smooth_with_mark)
				{
					drawing::PolyLine line;
					//line.Set_Position(mCanvas_WidthOff, mCanvas_HeightOff);
					for (const auto& val : signal.second.mPlots_Values) {

						line.Add_Point(
							mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) * ((val.device_time - min_x) / (max_x - min_x)),
							mCanvas_HeightOff - (mCanvas_HeightOff * ((val.value - min_y)* desc.value_scale / (max_y - min_y))));
					}
					const auto& all_points = line.Get_Points();
					if (!all_points.empty())
						line.Set_Position(all_points[0].x, all_points[0].y);

					grp.Add<drawing::PolyLine>(line)
						.Set_Stroke_Width(1.2)
						.Set_Fill_Opacity(0)
						.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true));
				}
				else if (desc.visualization == scgms::NSignal_Visualization::step ||
					desc.visualization == scgms::NSignal_Visualization::step_with_mark)
				{
					double last_x = 0, last_y = mCanvas_HeightOff;

					drawing::PolyLine line;
					line.Set_Position(mCanvas_WidthOff, mCanvas_HeightOff);
					for (const auto& val : signal.second.mPlots_Values) {

						last_x = mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) * ((val.device_time - min_x) / (max_x - min_x));

						line.Add_Point(last_x, last_y);

						last_y = mCanvas_HeightOff - (mCanvas_HeightOff * (val.value * desc.value_scale / (max_y - min_y)));

						line.Add_Point(last_x, last_y);
					}

					// TODO: per segment
					line.Add_Point(opts.width, last_y);

					grp.Add<drawing::PolyLine>(line)
						.Set_Stroke_Width(1.2)
						.Set_Fill_Opacity(0)
						.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true));
				}

				if (desc.visualization == scgms::NSignal_Visualization::mark ||
					desc.visualization == scgms::NSignal_Visualization::smooth_with_mark ||
					desc.visualization == scgms::NSignal_Visualization::step_with_mark)
				{
					constexpr double Plus_Distance = 4.0;
					constexpr double Star_Distance = 4.5;
					constexpr double Cross_Distance = 4.0;
					constexpr double Rect_Distance = 3.0;
					constexpr double Diamond_Distance = 4.0;
					constexpr double Triangle_Distance = 3.8;

					for (const auto& val : signal.second.mPlots_Values) {

						const double base_x = mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) * ((val.device_time - min_x) / (max_x - min_x));
						const double base_y = mCanvas_HeightOff - (mCanvas_HeightOff * (val.value * desc.value_scale / (max_y - min_y)));

						if (desc.mark == scgms::NSignal_Mark::circle)
						{
							grp.Add<drawing::Circle>(base_x, base_y, 8)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0)
								.Set_Fill_Opacity(0.0);
						}
						else if (desc.mark == scgms::NSignal_Mark::dot)
						{
							grp.Add<drawing::Circle>(base_x, base_y, 4)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Fill_Color(RGBColor::From_UInt32(desc.fill_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0)
								.Set_Fill_Opacity(1.0);
						}
						else if (desc.mark == scgms::NSignal_Mark::minus)
						{
							grp.Add<drawing::Line>(base_x - Plus_Distance, base_y, base_x + Plus_Distance, base_y)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0);
						}
						else if (desc.mark == scgms::NSignal_Mark::plus)
						{
							grp.Add<drawing::Line>(base_x - Plus_Distance, base_y, base_x + Plus_Distance, base_y)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0);

							grp.Add<drawing::Line>(base_x, base_y - Plus_Distance, base_x, base_y + Plus_Distance)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0);
						}
						else if (desc.mark == scgms::NSignal_Mark::star)
						{
							grp.Add<drawing::Line>(base_x, base_y - Star_Distance, base_x, base_y + Star_Distance)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(1.5)
								.Set_Stroke_Opacity(1.0);

							grp.Add<drawing::Line>(base_x - Star_Distance * 0.866, base_y - Star_Distance * 0.5, base_x + Star_Distance * 0.866, base_y + Star_Distance * 0.5)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(1.5)
								.Set_Stroke_Opacity(1.0);

							grp.Add<drawing::Line>(base_x + Star_Distance * 0.866, base_y - Star_Distance * 0.5, base_x - Star_Distance * 0.866, base_y + Star_Distance * 0.5)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(1.5)
								.Set_Stroke_Opacity(1.0);
						}
						else if (desc.mark == scgms::NSignal_Mark::cross)
						{
							grp.Add<drawing::Line>(base_x - Cross_Distance, base_y - Cross_Distance, base_x + Cross_Distance, base_y + Cross_Distance)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0);

							grp.Add<drawing::Line>(base_x - Cross_Distance, base_y + Cross_Distance, base_x + Cross_Distance, base_y - Cross_Distance)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0);
						}
						else if (desc.mark == scgms::NSignal_Mark::rectangle)
						{
							grp.Add<drawing::Rectangle>(base_x - Rect_Distance, base_y - Rect_Distance, 2 * Rect_Distance, 2 * Rect_Distance)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Fill_Color(RGBColor::From_UInt32(desc.fill_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0)
								.Set_Fill_Opacity(1.0);
						}
						else if (desc.mark == scgms::NSignal_Mark::diamond)
						{
							grp.Add<drawing::Polygon>(base_x - Diamond_Distance, base_y)
								.Add_Point(base_x - Diamond_Distance, base_y)
								.Add_Point(base_x, base_y - Diamond_Distance)
								.Add_Point(base_x + Diamond_Distance, base_y)
								.Add_Point(base_x, base_y + Diamond_Distance)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Fill_Color(RGBColor::From_UInt32(desc.fill_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0)
								.Set_Fill_Opacity(1.0);;
						}
						else if (desc.mark == scgms::NSignal_Mark::triangle)
						{
							grp.Add<drawing::Polygon>(base_x, base_y - Triangle_Distance)
								.Add_Point(base_x, base_y - Triangle_Distance)
								.Add_Point(base_x + Triangle_Distance * 0.866, base_y + Triangle_Distance * 0.5)
								.Add_Point(base_x - Triangle_Distance * 0.866, base_y + Triangle_Distance * 0.5)
								.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
								.Set_Fill_Color(RGBColor::From_UInt32(desc.fill_color, true))
								.Set_Stroke_Width(2)
								.Set_Stroke_Opacity(1.0)
								.Set_Fill_Opacity(1.0);;
						}
					}

				}

			}

		}
	}

	// render prepared canvas to string
	CSVG_Renderer renderer(opts.width, opts.height, target);
	draw.Render(renderer);

	return NDrawing_Error::Ok;
}
