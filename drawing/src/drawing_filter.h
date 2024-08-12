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

#include "drawing/Containers/Value.h"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_set>
#include <set>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Drawing filter class utilizing the code for generating SVGs based on input data
 */
class CDrawing_Filter : public scgms::CBase_Filter, public scgms::IDrawing_Filter_Inspection {
	protected:
		// stored AGP SVG
		std::string mAGP_SVG;
		// stored clark grid SVG
		std::string mClark_SVG;
		// stored day graph SVG
		std::string mDay_SVG;
		// stored graph SVG
		std::string mGraph_SVG;
		// stored parkes grid SVG for type 1
		std::string mParkes_type1_SVG;
		// stored parkes grid SVG for type 2
		std::string mParkes_type2_SVG;
		// stored ECDF SVG
		std::string mECDF_SVG;

		// stored profile - glucose SVG
		std::string mProfile_Glucose_SVG;
		// stored profile - bolus carbs SVG
		std::string mProfile_Carbs_SVG;
		// stored profile - basal SVG
		std::string mProfile_Insulin_SVG;

		// AGP file path
		filesystem::path mAGP_FilePath;
		// Clark file path
		filesystem::path mClark_FilePath;
		// day plot file path
		filesystem::path mDay_FilePath;
		// graph file path
		filesystem::path mGraph_FilePath;
		// Parkes file path
		filesystem::path mParkes_FilePath;
		// ECDF file path
		filesystem::path mECDF_FilePath;

		// input data changed (external state to indicate future redraw intent)
		std::atomic<bool> mChanged = false;
		// internal data changed flag (to not redraw multiple times)
		bool mChangedInternal = false;
		// mutex guard for changed variable
		std::mutex mChangedMtx;
		// scheduler condition variable
		std::condition_variable mSchedCv;
		// mutex guard for retrieving image data
		mutable std::mutex mRetrieveMtx;

		// localization map
		LocalizationMap mLocaleMap;
		// data map stored for drawing code; TODO: support more segments
		DataMap mDataMap;
		// maximum value in graphs; TODO: support more segments
		double mGraphMaxValue;

		// input data from pipe
		std::map<GUID, std::map<uint64_t, ValueVector>> mInputData;
		// calculated signal GUID-name map
		std::map<GUID, std::string> mCalcSignalNameMap;
		// cached map for fast mapping of reference signals to calc'd ones
		std::map<GUID, GUID> mReferenceForCalcMap;

		// markers for segment start/ends
		ValueVector mSegmentMarkers;
		// markes for parameters change
		std::map<GUID, std::map<uint64_t, ValueVector>> mParameterChanges;

		// configured canvas width
		int mCanvasWidth = 1024;
		// configured canvas height
		int mCanvasHeight = 768;

		std::set<GUID> mSignalsBeingReset;

	protected:
		// if the inputs changed, redraw SVGs
		bool Redraw_If_Changed(const std::unordered_set<uint64_t> &segmentIds = {}, const std::set<GUID> &signalIds = {});
		// redraw (without locking!)
		bool Force_Redraw(const std::unordered_set<uint64_t> &segmentIds = {}, const std::set<GUID> &signalIds = {});

		// fills given localization map with translation constants
		void Fill_Localization_Map(LocalizationMap& locales);
		// sets locale title parameter to given title string
		void Set_Locale_Title(LocalizationMap& locales, std::wstring title) const;

		// generates graphs for input values
		void Generate_Graphs(DataMap& valueMap, double maxValue, LocalizationMap& locales);
		// stores string to file path
		void Store_To_File(std::string& str, const filesystem::path& filePath);

		// resets stored values and parameter changes for given signal (must be valid signal id)
		void Reset_Signal(const GUID& signal_id, const uint64_t segment_id);

		// prepares data map for drawing
		void Prepare_Drawing_Map(const std::unordered_set<uint64_t> &segmentIds = {}, const std::set<GUID> &signalIds = {});

		HRESULT Get_Plot(const std::string &plot, refcnt::IVector_Container<char> *svg) const;

		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CDrawing_Filter(scgms::IFilter *output);
		virtual ~CDrawing_Filter() = default;

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;
	
		virtual HRESULT IfaceCalling New_Data_Available() override final;
		virtual HRESULT IfaceCalling Draw(scgms::TDrawing_Image_Type type, scgms::TDiagnosis diagnosis, refcnt::str_container *svg, refcnt::IVector_Container<uint64_t> *segmentIds, refcnt::IVector_Container<GUID> *signalIds) override final;
};

#pragma warning( pop )
