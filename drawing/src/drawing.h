#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include "drawing/Containers/Diff2.h"
#include "drawing/Containers/Diff3.h"
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
class CDrawing_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		// scheduler thread
		std::unique_ptr<std::thread> mSchedulerThread;

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

		// diagnosis flag
		uint8_t mDiagnosis;

		// redraw period [ms]
		int64_t mRedrawPeriod;
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
		int mCanvasWidth;
		// configured canvas height
		int mCanvasHeight;

		// internal running indicator
		bool mRunning;

		// thread function
		void Run_Main();
		// scheduler thread function
		void Run_Scheduler();

		// fills given localization map with translation constants
		void Fill_Localization_Map(LocalizationMap& locales);
		// sets locale title parameter to given title string
		void Set_Locale_Title(LocalizationMap& locales, std::wstring title) const;

		// generates graphs for input values
		void Generate_Graphs(DataMap& valueMap, double maxValue, LocalizationMap& locales);
		// stores string to file path
		void Store_To_File(std::string& str, std::wstring& filePath);

		// prepares data map for drawing
		void Prepare_Drawing_Map();

		// there should be only one instance of drawing filter
		static std::atomic<CDrawing_Filter*> mInstance;

	public:
		CDrawing_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);

		// retrieves generated AGP SVG
		const char* Get_SVG_AGP() const;
		// retrieves generated Clark SVG
		const char* Get_SVG_Clark() const;
		// retrieves generated day SVG
		const char* Get_SVG_Day() const;
		// retrieves generated graph SVG
		const char* Get_SVG_Graph() const;
		// retrieves generated Parkes SVG
		const char* Get_SVG_Parkes(bool type1 = true) const;

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;

		static CDrawing_Filter* Get_Instance();
};

using TGet_SVG = HRESULT(IfaceCalling*)(refcnt::wstr_container*);

#pragma warning( pop )