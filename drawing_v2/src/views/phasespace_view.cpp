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

#include "phasespace_view.h"

#include <iomanip>
#include <unordered_set>
#include <cmath>
#include <algorithm>

#include <scgms/iface/UIIface.h>
#include <scgms/rtl/UILib.h>
#include <scgms/utils/drawing/IRenderer.h>
#include <scgms/utils/drawing/SVGRenderer.h>
#include <scgms/utils/string_utils.h>

#include <scgms/utils/DebugHelper.h>

#undef min
#undef max

NDrawing_Error CPhase_Space_View::Draw(std::string& target, const TDraw_Options_Local& opts, const IDrawing_Data_Source& source) {

	if (opts.reference_signal_ids.size() == 0) {
		return NDrawing_Error::Not_Enough_Values;
	}

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
	double min_y = std::numeric_limits<double>::max();
	double max_y = -std::numeric_limits<double>::max();

	auto vector_contains_guid = [](const std::vector<GUID>& vec, const GUID& val) {
		return std::find(vec.begin(), vec.end(), val) != vec.end();
	};

	for (const auto seg_id : opts.segment_ids) {
		const TPlot_Segment& segment = source.Get_Segment(seg_id);

		for (const auto& signal : segment.mPlots_Signals) {
			if (!vector_contains_guid(opts.signal_ids, signal.first)) {
				continue;
			}

			scgms::TSignal_Descriptor desc = scgms::Null_Signal_Descriptor;

			if (scgms::get_signal_descriptor_by_id(signal.second.signal_id, desc)) {
				for (const auto& val : signal.second.mPlots_Values) {
					min_x = std::min(min_x, val.value * desc.value_scale);
					max_x = std::max(max_x, val.value * desc.value_scale);
				}
			}
		}

		// same for reference signals
		for (const auto& signal : segment.mPlots_Signals) {
			if (!vector_contains_guid(opts.reference_signal_ids, signal.first)) {
				continue;
			}
			scgms::TSignal_Descriptor desc = scgms::Null_Signal_Descriptor;
			if (scgms::get_signal_descriptor_by_id(signal.second.signal_id, desc)) {
				for (const auto& val : signal.second.mPlots_Values) {
					min_y = std::min(min_y, val.value * desc.value_scale);
					max_y = std::max(max_y, val.value * desc.value_scale);
				}
			}
		}
	}

	if (max_x < min_x || max_y < min_y) {
		return NDrawing_Error::Not_Enough_Values;
	}

	constexpr double Left_Margin = 0.05;
	constexpr double Right_Margin = 0.05;
	constexpr double Top_Margin = 0.1;
	constexpr double Bottom_Margin = 0.05;

	// add 5 % margin to all min and max values
	min_x -= (max_x - min_x) * Left_Margin;
	max_x += (max_x - min_x) * Right_Margin;
	min_y -= (max_y - min_y) * Bottom_Margin;
	max_y += (max_y - min_y) * Top_Margin;

	size_t total_x_label_cnt = static_cast<size_t>( (static_cast<double>(opts.width) - mCanvas_WidthOff) / 150.0 );
	total_x_label_cnt = std::clamp(total_x_label_cnt, static_cast<size_t>(4), static_cast<size_t>(18));

	double x_label_step_val = (max_x - min_x) / static_cast<double>(total_x_label_cnt);
	double x_label_step_px = (opts.width - mCanvas_WidthOff) * x_label_step_val / (max_x - min_x);

	// descriptions on X axis
	for (size_t i = 0; i < total_x_label_cnt; i++) {
		grp.Add<drawing::Text>(
				mCanvas_WidthOff + static_cast<double>(i) * x_label_step_px,
				mCanvas_HeightOff + 20,
				utility::Format_Decimal(min_x + static_cast<double>(i) * x_label_step_val, 1)
			)
			.Set_Font_Size(12) // TODO: font scaling proportionally to drawing size
			.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

		// draw vertical lines
		grp.Add<drawing::Line>(
			mCanvas_WidthOff + static_cast<double>(i) * x_label_step_px,
			mCanvas_HeightOff - 1,
			mCanvas_WidthOff + static_cast<double>(i) * x_label_step_px,
			0
			)
			.Set_Stroke_Color(RGBColor::From_HTML_Color("#D8D8D8"))
			.Set_Stroke_Opacity(1.0)
			.Set_Stroke_Width(1.0);
	}

	grp.Add<drawing::Text>(mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) / 2, mCanvas_HeightOff + 50, "Reference signal")
		.Set_Font_Size(15)
		.Set_Anchor(drawing::Text::TextAnchor::MIDDLE);

	size_t total_y_label_cnt = static_cast<size_t>((static_cast<double>(opts.height) - mCanvas_HeightOff) / 150.0);
	total_y_label_cnt = std::clamp(total_y_label_cnt, static_cast<size_t>(4), static_cast<size_t>(18));

	double y_label_step_val = (max_y - min_y) / static_cast<double>(total_y_label_cnt);
	double y_label_step_px = (opts.height - mCanvas_HeightOff) * y_label_step_val / (max_y - min_y);

	// descriptions on Y axis
	{
		double yf = y_label_step_val;
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

			yf += y_label_step_val;
		} while (yf <= max_y);
	}

	std::unordered_set<GUID> usedSignals;

	std::unordered_map<GUID, GUID> signal_to_reference_id;

	for (size_t si = 0; si < opts.signal_ids.size() && si < opts.reference_signal_ids.size(); si++) {
		signal_to_reference_id[opts.signal_ids[si]] = opts.reference_signal_ids[si];
	}

	double descriptionY = 20;
	for (const auto seg_id : opts.segment_ids) {

		const TPlot_Segment& segment = source.Get_Segment(seg_id);

		for (const auto& signal : segment.mPlots_Signals) {

			if (!vector_contains_guid(opts.signal_ids, signal.first)) {
				continue;
			}

			// find a reference signal ID
			auto ref_signal_id_itr = signal_to_reference_id.find(signal.first);
			if (ref_signal_id_itr == signal_to_reference_id.end()) {
				continue;
			}

			auto ref_signal_id = ref_signal_id_itr->second;

			auto ref_signal_itr = segment.mPlots_Signals.find(ref_signal_id);
			if (ref_signal_itr == segment.mPlots_Signals.end()) {
				continue;
			}

			const auto& ref_signal = ref_signal_itr->second;

			scgms::TSignal_Descriptor desc = scgms::Null_Signal_Descriptor;
			scgms::TSignal_Descriptor ref_desc = scgms::Null_Signal_Descriptor;

			if (!scgms::get_signal_descriptor_by_id(signal.second.signal_id, desc)) {
				continue;
			}
			if (!scgms::get_signal_descriptor_by_id(ref_signal_id, ref_desc)) {
				continue;
			}

			// draw description text only on first appearance
			if (usedSignals.find(desc.id) == usedSignals.end()) {
				grp.Add<drawing::Text>(mCanvas_WidthOff + 20, descriptionY, Narrow_WChar(desc.signal_description) + " [" + Narrow_WChar(desc.unit_description) + "]")
					.Set_Font_Size(10)
					.Set_Anchor(drawing::Text::TextAnchor::START)
					.Set_Fill_Color(RGBColor::From_UInt32(desc.fill_color, true));

				usedSignals.insert(desc.id);

				descriptionY += 20;
			}

			constexpr double Diamond_Distance = 2.0;

			auto find_matching_ref_value = [&ref_signal, &ref_signal_id](const TPlot_Value& val) {
				for (const auto& ref_val : ref_signal.mPlots_Values) {
					if (val.device_time == ref_val.device_time) {
						return ref_val;
					}
				}
				return TPlot_Value{ 0.0, 0.0 };
			};

			bool firstValue = true;
			double lastX = 0, lastY = 0;

			for (const auto& val : signal.second.mPlots_Values) {

				auto matching = find_matching_ref_value(val);
				if (matching.device_time == 0.0) {
					continue;
				}

				const double base_x = mCanvas_WidthOff + (opts.width - mCanvas_WidthOff) * ((val.value * desc.value_scale - min_x) / (max_x - min_x));
				const double base_y = mCanvas_HeightOff - (mCanvas_HeightOff * (matching.value * ref_desc.value_scale / (max_y - min_y)));

				grp.Add<drawing::Circle>(base_x, base_y, 1.0)
					.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
					.Set_Fill_Color(RGBColor::From_UInt32(desc.fill_color, true))
					.Set_Stroke_Width(0.5)
					.Set_Stroke_Opacity(1.0)
					.Set_Fill_Opacity(1.0);

				if (!firstValue) {
					grp.Add<drawing::Line>(lastX, lastY, base_x, base_y)
						.Set_Stroke_Color(RGBColor::From_UInt32(desc.stroke_color, true))
						.Set_Stroke_Width(1.0)
						.Set_Stroke_Opacity(0.3);
				}
				else {
					firstValue = false;
				}

				lastX = base_x;
				lastY = base_y;
			}
		}
	}

	// render prepared canvas to string
	CSVG_Renderer renderer(opts.width, opts.height, target);
	draw.Render(renderer);

	return NDrawing_Error::Ok;
}
