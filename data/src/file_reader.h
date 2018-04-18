#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include "fileloader/Structures.h"
#include "fileloader/FormatAdapter.h"
#include "fileloader/Extractor.h"

#include <memory>
#include <thread>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

using TValue_Vector = std::vector<CMeasured_Value*>;

/*
 * Filter class for loading and extracting file, and sending values to chain
 */
class CFile_Reader : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		// original filename from configuration
		std::wstring mFileName;
		// segment spacing as julian date
		double mSegmentSpacing;
		// merged extracted values from given file
		std::vector<TValue_Vector> mMergedValues;

		// reader thread
		std::unique_ptr<std::thread> mReaderThread;

		// reader main method
		void Run_Reader();
		// thread function
		void Run_Main();

		// send event to filter chain
		HRESULT Send_Event(glucose::NDevice_Event_Code code, double device_time, int64_t logical_time, uint64_t segment_id, const GUID* signalId = nullptr, double value = 0.0);
		// merge values from extraction result to internal vector
		void Merge_Values(ExtractionResult& result);

	public:
		CFile_Reader(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);
		virtual ~CFile_Reader();

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
};

#pragma warning( pop )
