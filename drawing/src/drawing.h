/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
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
class CDrawing_Filter : public virtual glucose::IFilter, public virtual glucose::IDrawing_Filter_Inspection, public virtual refcnt::CReferenced {
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

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

		// thread function
		void Run_Main();

		// if the inputs changed, redraw SVGs
		bool Redraw_If_Changed(const std::unordered_set<uint64_t> &segmentIds = {}, const std::set<GUID> &signalIds = {});

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
		CDrawing_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CDrawing_Filter() {};

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;

		virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;

		virtual HRESULT IfaceCalling Draw(glucose::TDrawing_Image_Type type, glucose::TDiagnosis diagnosis, refcnt::str_container *svg, refcnt::IVector_Container<uint64_t> *segmentIds, refcnt::IVector_Container<GUID> *signalIds) override;
};

#pragma warning( pop )
