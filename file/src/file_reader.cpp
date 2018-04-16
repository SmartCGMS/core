#include "file_reader.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/rattime.h"

#include "loader/FormatRuleLoader.h"
#include "loader/FormatRecognizer.h"
#include "loader/Extractor.h"

#include <iostream>
#include <chrono>

namespace file_reader
{
	const GUID File_Reader_Device_GUID = { 0x00000002, 0x0002, 0x0002, { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

	// default value spacing in segments - this space determines when the segment ends and starts new
	constexpr double Default_Segment_Spacing = 600.0 * 1000.0 * InvMSecsPerDay;
}

CFile_Reader::CFile_Reader(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
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

HRESULT CFile_Reader::Send_Event(glucose::NDevice_Event_Code code, double device_time, int64_t logical_time, uint64_t segment_id, const GUID* signalId, double value)
{
	glucose::TDevice_Event evt;

	evt.device_id = file_reader::File_Reader_Device_GUID;
	evt.device_time = device_time;
	evt.event_code = code;
	evt.logical_time = logical_time;
	evt.level = value;
	evt.segment_id = segment_id;
	if (signalId)
		evt.signal_id = *signalId;

	return mOutput->send(&evt);
}

void CFile_Reader::Run_Reader()
{
	CMeasured_Value* last = nullptr;
	bool segmentStarted = false;
	int64_t logicalTime = 0;

	time_t t_now;
	double valDate;
	double dateCorrection;

	bool isError = false;

	uint32_t currentSegmentId = 0;

	// for each file loaded...
	for (auto& vals : mMergedValues)
	{
		last = nullptr;
		if (isError)
			break;

		// for every value in file...
		for (CMeasured_Value* cur : vals)
		{
			// stop/start a new segment, if needed (first value in file, or the current value is further away from previous, than spacing limit)
			if (last == nullptr || cur->mMeasuredAt - last->mMeasuredAt > mSegmentSpacing)
			{
				// end previous segment, if started
				if (segmentStarted && last)
					Send_Event(glucose::NDevice_Event_Code::Time_Segment_Stop, last->mMeasuredAt + dateCorrection, logicalTime++, currentSegmentId);

				// base correction is used as value for shifting the time segment (its values) from past to present
				// adding this to every value will cause the segment to begin with the first value as if it was measured just now
				// and every next value will be shifted by exactly the same amount of time, so the spacing is preserved
				// this allows artificial slowdown filter to simulate real-time measurement without need of real device

				// get new correction value with each segment start
				t_now = static_cast<time_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
				dateCorrection = Unix_Time_To_Rat_Time(t_now) - cur->mMeasuredAt;
				currentSegmentId++;

				Send_Event(glucose::NDevice_Event_Code::Time_Segment_Start, cur->mMeasuredAt + dateCorrection, logicalTime, currentSegmentId);
				logicalTime++;
				segmentStarted = true;
			}

			valDate = cur->mMeasuredAt + dateCorrection;

			// now go through every field, check for value presence, and send it to chain

			bool errorRes = false;

			if (cur->mIst.has_value())
				errorRes |= (Send_Event(glucose::NDevice_Event_Code::Level, valDate, logicalTime++, currentSegmentId, &glucose::signal_IG, cur->mIst.value()) != S_OK);
			if (cur->mIsig.has_value())
				errorRes |= (Send_Event(glucose::NDevice_Event_Code::Level, valDate, logicalTime++, currentSegmentId, &glucose::signal_ISIG, cur->mIsig.value()) != S_OK);
			if (cur->mBlood.has_value())
				errorRes |= (Send_Event(glucose::NDevice_Event_Code::Level, valDate, logicalTime++, currentSegmentId, &glucose::signal_BG, cur->mBlood.value()) != S_OK);
			if (cur->mInsulin.has_value())
				errorRes |= (Send_Event(glucose::NDevice_Event_Code::Level, valDate, logicalTime++, currentSegmentId, &glucose::signal_Insulin, cur->mInsulin.value()) != S_OK);
			if (cur->mCarbohydrates.has_value())
				errorRes |= (Send_Event(glucose::NDevice_Event_Code::Level, valDate, logicalTime++, currentSegmentId, &glucose::signal_Carb_Intake, cur->mCarbohydrates.value()) != S_OK);
			if (cur->mCalibration.has_value())
				errorRes |= (Send_Event(glucose::NDevice_Event_Code::Calibrated, valDate, logicalTime++, currentSegmentId, &glucose::signal_Calibration, cur->mCalibration.value()) != S_OK);

			if (errorRes)
			{
				isError = true;
				break;
			}

			last = cur;
		}

		// end segment if it was previously started
		if (segmentStarted && last)
		{
			if (Send_Event(glucose::NDevice_Event_Code::Time_Segment_Stop, last->mMeasuredAt + dateCorrection, logicalTime++, currentSegmentId) != S_OK)
				isError = true;
			segmentStarted = false;
		}
	}
}

void CFile_Reader::Run_Main()
{
	glucose::TDevice_Event evt;

	while (mInput->receive(&evt) == S_OK)
	{
		// just fall through in main filter thread
		// there also may be some control code handling (i.e. pausing value sending, etc.)

		if (mOutput->send(&evt) != S_OK)
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

HRESULT CFile_Reader::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	glucose::TFilter_Parameter *cbegin, *cend;
	if (configuration->get(&cbegin, &cend) != S_OK)
		return E_FAIL;

	for (glucose::TFilter_Parameter* cur = cbegin; cur < cend; cur += 1)
	{
		wchar_t *begin, *end;
		if (cur->config_name->get(&begin, &end) != S_OK)
			continue;

		std::wstring confname{ begin, end };

		if (confname == rsInput_Values_File)
		{
			if (cur->wstr->get(&begin, &end) != S_OK)
				continue;

			mFileName = std::wstring{ begin, end };
		}
		else if (confname == rsInput_Segment_Spacing)
			mSegmentSpacing = static_cast<double>(cur->int64) * 1000.0 * InvMSecsPerDay;
	}

	if (mFileName.empty())
		return ENOENT;

	ExtractionResult values;

	// loader scope
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
	}

	Merge_Values(values);

	mReaderThread = std::make_unique<std::thread>(&CFile_Reader::Run_Reader, this);
	Run_Main();

	return S_OK;
};
