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

#include "drawing_filter_v2.h"

#include "../views/graph_view.h"
#include "../views/cvga.h"

#include <set>
#include <cmath>

// vector of supported views
std::vector<scgms::TPlot_Descriptor> CDrawing_Filter_v2::mAvailable_Plots = {
	{ scgms::dcGraph, L"Graph (v2)" },
	{ scgms::dcCVGA, L"CVGA (v2)"},
};

CDrawing_Filter_v2::CDrawing_Filter_v2(scgms::IFilter *output) : CBase_Filter(output) {

	// register known views
	Register_View<CGraph_View>(scgms::dcGraph);
	Register_View<CCVGA_View>(scgms::dcCVGA);
}

HRESULT IfaceCalling CDrawing_Filter_v2::QueryInterface(const GUID*  riid, void ** ppvObj) {

	if (Internal_Query_Interface<scgms::IDrawing_Filter_Inspection_v2>(scgms::IID_Drawing_Filter_Inspection_v2, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

HRESULT IfaceCalling CDrawing_Filter_v2::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	mDefault_Canvas_Width = static_cast<int>(configuration.Read_Int(drawing_filter_v2::rsConfig_Drawing_Width, mDefault_Canvas_Width));
	mDefault_Canvas_Height = static_cast<int>(configuration.Read_Int(drawing_filter_v2::rsConfig_Drawing_Height, mDefault_Canvas_Height));

	if (mDefault_Canvas_Width < 0 || mDefault_Canvas_Height < 0) {
		error_description.push(L"Invalid canvas dimensions");
		return E_INVALIDARG;
	}

	return S_OK;
}

HRESULT IfaceCalling CDrawing_Filter_v2::Do_Execute(scgms::UDevice_Event event) {

	// store every level
	if (event.event_code() == scgms::NDevice_Event_Code::Level) {

		std::map<GUID, TPlot_Signal>::iterator sitr;

		auto itr = mPlots_Segments.find(event.segment_id());
		if (itr == mPlots_Segments.end())
		{
			auto [itr, success] = mPlots_Segments.insert({ event.segment_id(), {event.segment_id(), {}} });
			if (!success)
				return E_FAIL;

			auto [sitr2, success2] = itr->second.mPlots_Signals.insert({ event.signal_id(), { event.signal_id(), {} } });
			if (!success2)
				return E_FAIL;

			sitr = sitr2;
		}
		else
		{
			sitr = itr->second.mPlots_Signals.find(event.signal_id());
			if (sitr == itr->second.mPlots_Signals.end())
			{
				auto [sitr2, success] = itr->second.mPlots_Signals.insert({ event.signal_id(), { event.signal_id(), {} } });
				if (!success)
					return E_FAIL;

				sitr = sitr2;
			}
		}

		sitr->second.mPlots_Values.push_back({
			event.device_time(),
			event.level()
			});

		// increment clock to indicate new version is available; TODO: increment only if no external code requested Draw or Logical_Clock (do not increment if there is no need to increment)
		mLogical_Clock++;
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Warm_Reset)
		mLogical_Clock = 0;

	return mOutput.Send(event);
}

HRESULT IfaceCalling CDrawing_Filter_v2::Logical_Clock(ULONG* clock)
{
	// requester has current state = no new data, success, but no update is needed
	if (*clock == mLogical_Clock) {
		return S_FALSE;
	}
	// requester has older state = new data available, success, make him request new data
	else if (*clock < mLogical_Clock) {
		*clock = mLogical_Clock;
		return S_OK;
	}

	// we are the source of change, so requester should not have a newer clock than us = return error
	return E_ILLEGAL_STATE_CHANGE;
}

HRESULT IfaceCalling CDrawing_Filter_v2::Get_Capabilities(refcnt::IVector_Container<scgms::TPlot_Descriptor>* descs) const
{
	return descs->add(mAvailable_Plots.data(), mAvailable_Plots.data() + mAvailable_Plots.size());
}

HRESULT IfaceCalling CDrawing_Filter_v2::Get_Available_Segments(refcnt::IVector_Container<uint64_t>* segments) const
{
	if (mPlots_Segments.empty())
		return S_FALSE;

	for (auto s : mPlots_Segments) {
		auto hres = segments->add(&s.second.segment_id, &s.second.segment_id + 1);
		if (hres != S_OK)
			return hres;
	}

	return S_OK;
}

HRESULT IfaceCalling CDrawing_Filter_v2::Get_Available_Signals(uint64_t segment_id, refcnt::IVector_Container<GUID>* out_signals)
{
	if (mPlots_Segments.empty())
		return S_FALSE;

	// find segment
	auto itr = mPlots_Segments.find(segment_id);
	if (itr == mPlots_Segments.end())
		return S_FALSE;

	const auto& seg = itr->second;

	// copy signal GUIDs
	for (auto s : seg.mPlots_Signals) {
		auto hres = out_signals->add(&s.second.signal_id, &s.second.signal_id + 1);
		if (hres != S_OK)
			return hres;
	}

	return S_OK;
}

HRESULT IfaceCalling CDrawing_Filter_v2::Draw(const GUID* plot_id, refcnt::str_container* svg, const scgms::TDraw_Options* options)
{
	// find view handler
	auto view_generator = mViews.find(*plot_id);

	// if we know such handler...
	if (view_generator != mViews.end())
	{
		// construct local options struct to allow view handler to access it in C++ way
		TDraw_Options_Local opts;
		opts.width = options->width <= 0 ? mDefault_Canvas_Width : options->width;
		opts.height = options->height <= 0 ? mDefault_Canvas_Height : options->height;
		
		// copy segment IDs
		if (options->segments == nullptr)
		{
			for (auto& seg : mPlots_Segments)
				opts.segment_ids.push_back(seg.first);
		}
		else
			opts.segment_ids.assign(options->segments, options->segments + options->segment_count);

		// copy signal GUIDs
		if (options->in_signals == nullptr)
		{
			for (auto& seg : mPlots_Segments)
			{
				for (auto& sig : seg.second.mPlots_Signals)
					opts.signal_ids.insert(sig.first);
			}
		}
		else
			opts.signal_ids.insert(options->in_signals, options->in_signals + options->signal_count);

		// copy reference signal GUIDs
		if (options->reference_signals != nullptr)
			opts.reference_signal_ids.insert(options->reference_signals, options->reference_signals + options->signal_count);

		// render
		std::string drawing;
		if (view_generator->second->Draw(drawing, opts, *this) != NDrawing_Error::Ok)
			return E_FAIL;

		return svg->set(drawing.data(), drawing.data() + drawing.size());
	}

	// we don't know such plot
	return E_NOTIMPL;
}

const TPlot_Segment& CDrawing_Filter_v2::Get_Segment(const size_t id) const
{
	// purposefully used "at" to generate exception when there's no such segment
	return mPlots_Segments.at(id);
}
