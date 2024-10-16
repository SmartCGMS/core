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

#pragma once

#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/UILib.h>
#include <scgms/rtl/rattime.h>

#include "../descriptor.h"
#include "../views/view_base.h"

#include <vector>
#include <map>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Reference implementation of drawing_v2 interface - new generation drawing filter
 */
class CDrawing_Filter_v2 : public scgms::CBase_Filter, public scgms::IDrawing_Filter_Inspection_v2, public IDrawing_Data_Source {

	private:
		// container of registered view
		struct TRegistered_View {
			GUID id;
			std::unique_ptr<CDrawing_View_Base> handler;
			std::wstring output_filename_config_option_name;
		};

	private:
		// default canvas size
		int mDefault_Canvas_Width = 800;
		int mDefault_Canvas_Height = 600;

		// filenames for SVG outputs
		std::map<GUID, std::wstring> mOutput_Files;

		// Lamport clock of drawings
		ULONG mLogical_Clock = 0;

		// available plots in this implementation
		static std::array<scgms::TPlot_Descriptor, 2> mAvailable_Plots;

		// registered view handlers (generators)
		std::map<GUID, TRegistered_View> mViews;

		// stored data
		std::map<uint64_t, TPlot_Segment> mPlots_Segments;

	private:
		// registers a view handler (instantiates the handler)
		template<typename T, typename... Args>
		void Register_View(const GUID& id, const std::wstring& filename_option_name, Args... args) {
			mViews.insert({
				id, TRegistered_View{ id, std::make_unique<T>(std::forward<Args>(args)...), filename_option_name }
			});
		}

	protected:
		// scgms::CBase_Filter iface
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

		void Render_All_To_File_Default();
		void Log_Error(const std::wstring& error_str);

	public:
		CDrawing_Filter_v2(scgms::IFilter *output);
		virtual ~CDrawing_Filter_v2() = default;
	
		// scgms::IReferenced iface
		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;

		// scgms::ILogical_Clock iface
		virtual HRESULT IfaceCalling Logical_Clock(ULONG* clock) override;

		// scgms::IDrawing_Filter_Inspection_v2 iface
		virtual HRESULT IfaceCalling Get_Capabilities(refcnt::IVector_Container<scgms::TPlot_Descriptor>* descs) const override;
		virtual HRESULT IfaceCalling Get_Available_Segments(refcnt::IVector_Container<uint64_t>* segments) const override;
		virtual HRESULT IfaceCalling Get_Available_Signals(uint64_t segment_id, refcnt::IVector_Container<GUID>* out_signals) override;
		virtual HRESULT IfaceCalling Draw(const GUID* plot_id, refcnt::str_container* svg, const scgms::TDraw_Options* options) override;

		// IDrawing_Data_Source iface
		virtual const TPlot_Segment& Get_Segment(const size_t id) const override;
};

#pragma warning( pop )
