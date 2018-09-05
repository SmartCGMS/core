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

#include "fileloader/Structures.h"
#include "fileloader/FormatAdapter.h"
#include "fileloader/Extractor.h"

#include <memory>
#include <thread>
#include <list>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

using TValue_Vector = std::vector<CMeasured_Value*>;
// segment begin and end (indexes in given array/vector)
using TSegment_Limits = std::pair<size_t, size_t>;

/*
 * Filter class for loading and extracting file, and sending values to chain
 */
class CFile_Reader : public glucose::IFilter, public virtual refcnt::CReferenced {
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// original filename from configuration
		std::wstring mFileName;
		// segment spacing as julian date
		double mSegmentSpacing;
		// merged extracted values from given file
		std::vector<TValue_Vector> mMergedValues;
		// do we need to send shutdown after last value?
		bool mShutdownAfterLast = false;
		// minimum values in segment
		size_t mMinValueCount;
		// require both IG and BG values in a segment
		bool mRequireBG_IG;

		// reader thread
		std::unique_ptr<std::thread> mReaderThread;

		// reader main method
		void Run_Reader();
		// thread function
		void Run_Main();

		// send event to filter chain
		bool Send_Event(glucose::NDevice_Event_Code code, double device_time, uint64_t segment_id, const GUID* signalId = nullptr, double value = 0.0);
		// extracts file to value vector container
		HRESULT Extract(ExtractionResult &values);
		// merge values from extraction result to internal vector
		void Merge_Values(ExtractionResult& result);

		// resolves segments of given value vector
		void Resolve_Segments(TValue_Vector const& src, std::list<TSegment_Limits>& targetList) const;

	public:
		CFile_Reader(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CFile_Reader();

		virtual HRESULT Run(refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override;
};

#pragma warning( pop )
