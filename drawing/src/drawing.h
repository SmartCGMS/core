#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "drawing/Containers/Value.h"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

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

		// diagnosis flag
		uint8_t mDiagnosis = 0; //type 1?

		// input data changed
		bool mChanged;
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
		std::map<GUID, ValueVector> mInputData;
		// calculated signal GUID-name map
		std::map<GUID, std::string> mCalcSignalNameMap;

		// markers for segment start/ends
		ValueVector mSegmentMarkers;
		// markes for parameters change
		std::map<GUID, ValueVector> mParameterChanges;

		// configured canvas width
		int mCanvasWidth = 1024;
		// configured canvas height
		int mCanvasHeight = 768;

		// thread function
		void Run_Main();

		// if the inputs changed, redraw SVGs
		bool Redraw_If_Changed();

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
		void Prepare_Drawing_Map();

		HRESULT Get_Plot(const std::string &plot, refcnt::IVector_Container<char> *svg) const;
	public:
		CDrawing_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CDrawing_Filter() {};

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;

		virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;

		virtual HRESULT IfaceCalling Draw(glucose::TDrawing_Image_Type type, glucose::TDiagnosis diagnosis, refcnt::str_container *svg) override;		
};

#pragma warning( pop )
