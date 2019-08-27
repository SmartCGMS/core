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
 * Univerzitni 8
 * 301 00, Pilsen
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

#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

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
class CDrawing_Filter : public glucose::ISynchronnous_Filter, public glucose::IDrawing_Filter_Inspection, public virtual refcnt::CReferenced {
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
		std::wstring mAGP_FilePath;
		// Clark file path
		std::wstring mClark_FilePath;
		// day plot file path
		std::wstring mDay_FilePath;
		// graph file path
		std::wstring mGraph_FilePath;
		// Parkes file path
		std::wstring mParkes_FilePath;
		// ECDF file path
		std::wstring mECDF_FilePath;

		// input data changed
		bool mChanged = false;
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
		void Store_To_File(std::string& str, std::wstring& filePath);

		// resets stored values and parameter changes for given signal (must be valid signal id)
		void Reset_Signal(const GUID& signal_id, const uint64_t segment_id);

		// prepares data map for drawing
		void Prepare_Drawing_Map(const std::unordered_set<uint64_t> &segmentIds = {}, const std::set<GUID> &signalIds = {});

		HRESULT Get_Plot(const std::string &plot, refcnt::IVector_Container<char> *svg) const;
	public:
		CDrawing_Filter();
		virtual ~CDrawing_Filter() = default;

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;

		virtual HRESULT Configure(glucose::IFilter_Configuration* configuration) override;
		virtual HRESULT Execute(glucose::IDevice_Event_Vector* events) override;

		virtual HRESULT IfaceCalling Draw(glucose::TDrawing_Image_Type type, glucose::TDiagnosis diagnosis, refcnt::str_container *svg, refcnt::IVector_Container<uint64_t> *segmentIds, refcnt::IVector_Container<GUID> *signalIds) override;
};

#pragma warning( pop )
