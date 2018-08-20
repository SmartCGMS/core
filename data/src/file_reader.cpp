#include "file_reader.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/rattime.h"

#include "fileloader/FormatRuleLoader.h"
#include "fileloader/FormatRecognizer.h"
#include "fileloader/Extractor.h"

#include <iostream>
#include <chrono>

namespace file_reader
{
	const GUID File_Reader_Device_GUID = { 0x00000002, 0x0002, 0x0002, { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

	// default value spacing in segments - this space determines when the segment ends and starts new
	constexpr double Default_Segment_Spacing = 600.0 * 1000.0 * InvMSecsPerDay;
}

CFile_Reader::CFile_Reader(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput(inpipe), mOutput(outpipe), mSegmentSpacing(file_reader::Default_Segment_Spacing)
{
	//
}

CFile_Reader::~CFile_Reader()
{
	// cleanup loaded values
	for (auto& vals : mMergedValues)
	{
		for (CMeasured_Value* cur : vals)
			delete cur;
	}
}

bool CFile_Reader::Send_Event(glucose::NDevice_Event_Code code, double device_time, uint64_t segment_id, const GUID* signalId, double value)
{
	glucose::UDevice_Event evt{ code };

	evt.device_id = file_reader::File_Reader_Device_GUID;
	evt.device_time = device_time;	
	evt.level = value;
	evt.segment_id = segment_id;
	if (signalId)
		evt.signal_id = *signalId;

	return mOutput.Send(evt);
}

void CFile_Reader::Resolve_Segments(TValue_Vector const& src, std::list<TSegment_Limits>& targetList) const
{
	//targetList.push_back(std::make_pair<size_t, size_t>(0, src.size()));

	size_t begin = 0;
	size_t end;

	// the first iteration just finds all segment candidates and limits the minimum count of levels in each segment, no additional filtering involved yet
	while (begin < src.size())
	{
		for (end = begin + 1; end < src.size(); end++)
		{
			// NOTE: we assume sorted values on input (see CFile_Reader::Merge_Values)
			if (end == src.size() - 1 || src[end]->mMeasuredAt - src[end - 1]->mMeasuredAt > mSegmentSpacing)
			{
				if (end - begin >= mMinValueCount)
					targetList.push_back({ begin, end });
				break;
			}
		}

		begin = end;
	}

	// filter segment without both BG or IG
	if (mRequireBG_IG)
	{
		bool igFound, bgFound;

		for (auto itr = targetList.begin(); itr != targetList.end(); )
		{
			igFound = false;
			bgFound = false;

			for (size_t i = itr->first; i < itr->second && (!igFound || !bgFound); i++)
			{
				if (src[i]->mBlood.has_value() || src[i]->mCalibration.has_value())
					bgFound = true;
				if (src[i]->mIst.has_value())
					igFound = true;
			}

			if (!igFound || !bgFound)
				itr = targetList.erase(itr);
			else
				itr++;
		}
	}
}

void CFile_Reader::Run_Reader()
{
	bool isError = false;
	uint32_t currentSegmentId = 0;

	// for each file loaded...
	for (const auto& vals : mMergedValues)
	{
		std::list<TSegment_Limits> resolvedSegments;
		Resolve_Segments(vals, resolvedSegments);

		// for every segment resolved...
		for (const TSegment_Limits& seg : resolvedSegments)
		{
			currentSegmentId++;
			Send_Event(glucose::NDevice_Event_Code::Time_Segment_Start, vals[seg.first]->mMeasuredAt, currentSegmentId);

			for (size_t i = seg.first; i < seg.second; i++)
			{
				const CMeasured_Value* cur = vals[i];

				const double valDate = cur->mMeasuredAt;

				// now go through every field, check for value presence, and send it to chain

				bool errorRes = false;

				if (cur->mIst.has_value())
					errorRes |= !Send_Event(glucose::NDevice_Event_Code::Level, valDate, currentSegmentId, &glucose::signal_IG, cur->mIst.value());
				if (cur->mIsig.has_value())
					errorRes |= !Send_Event(glucose::NDevice_Event_Code::Level, valDate, currentSegmentId, &glucose::signal_ISIG, cur->mIsig.value());
				if (cur->mBlood.has_value())
					errorRes |= !Send_Event(glucose::NDevice_Event_Code::Level, valDate, currentSegmentId, &glucose::signal_BG, cur->mBlood.value());
				if (cur->mInsulin.has_value())
					errorRes |= !Send_Event(glucose::NDevice_Event_Code::Level, valDate, currentSegmentId, &glucose::signal_Insulin, cur->mInsulin.value());
				if (cur->mCarbohydrates.has_value())
					errorRes |= !Send_Event(glucose::NDevice_Event_Code::Level, valDate, currentSegmentId, &glucose::signal_Carb_Intake, cur->mCarbohydrates.value());
				if (cur->mCalibration.has_value())
					errorRes |= !Send_Event(glucose::NDevice_Event_Code::Level, valDate, currentSegmentId, &glucose::signal_Calibration, cur->mCalibration.value());

				if (errorRes)
				{
					isError = true;
					break;
				}
			}

			if (!Send_Event(glucose::NDevice_Event_Code::Time_Segment_Stop, vals[seg.second - 1]->mMeasuredAt, currentSegmentId))
				isError = true;

			if (isError)
				break;
		}

		if (isError)
			break;
	}

	if (mShutdownAfterLast)
	{
		glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Shut_Down };

		evt.device_id = file_reader::File_Reader_Device_GUID;
		mOutput.Send(evt);
	}
}

void CFile_Reader::Run_Main() {

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
	
		// just fall through in main filter thread
		// there also may be some control code handling (i.e. pausing value sending, etc.)

		if (!mOutput.Send(evt))
			break;
	}

	if (mReaderThread->joinable())
		mReaderThread->join();
}

void CFile_Reader::Merge_Values(ExtractionResult& result)
{
	// prepare output vector
	mMergedValues.resize(result.SegmentValues.size());

	// go through all loaded values and push them to single vector containing all values
	for (size_t i = 0; i < result.SegmentValues.size(); i++)
	{
		for (size_t j = 0; j < result.SegmentValues[i].size(); j++)
			mMergedValues[i].push_back(result.SegmentValues[i][j]);

		for (size_t j = 0; j < result.SegmentBloodValues[i].size(); j++)
			mMergedValues[i].push_back(result.SegmentBloodValues[i][j]);

		for (size_t j = 0; j < result.SegmentMiscValues[i].size(); j++)
			mMergedValues[i].push_back(result.SegmentMiscValues[i][j]);

		// sort values by timestamp from ascending
		std::sort(mMergedValues[i].begin(), mMergedValues[i].end(), [](CMeasured_Value* a, CMeasured_Value* b) { return a->mMeasuredAt < b->mMeasuredAt; });
	}
}

HRESULT CFile_Reader::Extract(ExtractionResult &values)
{
	CFormat_Recognizer recognizer;
	CExtractor extractor;

	CFormat_Rule_Loader loader(recognizer, extractor);
	if (!loader.Load())
		return E_FAIL;

	values.Value_Count = 0;
	values.SegmentValues.clear();

	// TODO: support more files at once
	values.SegmentValues.resize(1);
	values.SegmentBloodValues.resize(1);
	values.SegmentMiscValues.resize(1);

	CFormat_Adapter sfile;
	std::string format = recognizer.Recognize_And_Open(mFileName, sfile);

	// unrecognized format or error loading file
	if (format == "" || sfile.Get_Error())
		return E_FAIL;

	// extract data and fill extraction result
	if (!extractor.Extract(format, sfile, values, 0))
		return E_FAIL;

	return S_OK;
}

HRESULT CFile_Reader::Run(refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	auto conf = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);

	mFileName = conf.Read_String(rsInput_Values_File);
	mSegmentSpacing = conf.Read_Int(rsInput_Segment_Spacing) * 1000.0 * InvMSecsPerDay;
	mShutdownAfterLast = conf.Read_Bool(rsShutdown_After_Last);
	mMinValueCount = static_cast<size_t>(conf.Read_Int(rsMinimum_Segment_Levels));
	mRequireBG_IG = conf.Read_Bool(rsRequire_IG_BG);

	if (mFileName.empty())
		return ENOENT;

	ExtractionResult values;
	HRESULT rc = Extract(values);
	if (rc != S_OK)
		return rc;

	Merge_Values(values);

	mReaderThread = std::make_unique<std::thread>(&CFile_Reader::Run_Reader, this);
	Run_Main();

	return S_OK;
};
