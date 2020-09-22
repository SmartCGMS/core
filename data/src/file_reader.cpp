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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "file_reader.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/rtl/hresult.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/string_utils.h"

#include "fileloader/FormatRuleLoader.h"
#include "fileloader/FormatRecognizer.h"
#include "fileloader/Extractor.h"

#include <iostream>
#include <chrono>
#include <string>

namespace file_reader
{
	const GUID File_Reader_Device_GUID = { 0x78df0982, 0x705, 0x4c84, { 0xb0, 0xa3, 0xae, 0x40, 0xfe, 0x7e, 0x18, 0x7b } };	// {78DF0982-0705-4C84-B0A3-AE40FE7E187B}

	// default value spacing in segments - this space determines when the segment ends and starts new
	constexpr double Default_Segment_Spacing = 600.0 * 1000.0 * InvMSecsPerDay;
}

CFile_Reader::CFile_Reader(scgms::IFilter *output) : CBase_Filter(output), mSegmentSpacing(file_reader::Default_Segment_Spacing) {
	//
}

CFile_Reader::~CFile_Reader() {

	if (mReaderThread && mReaderThread->joinable())
		mReaderThread->join();

	// cleanup loaded values
	for (auto& vals : mMergedValues)
	{
		for (CMeasured_Value* cur : vals)
			delete cur;
	}
}

bool CFile_Reader::Send_Event(scgms::NDevice_Event_Code code, double device_time, uint64_t segment_id, const GUID& signalId, double value, const std::wstring& winfo)
{
	scgms::UDevice_Event evt{ code };

	evt.device_id() = file_reader::File_Reader_Device_GUID;
	evt.device_time() = device_time;
	if (evt.is_level_event())
		evt.level() = value;
	evt.segment_id() = segment_id;
	evt.signal_id() = signalId;
	if (evt.is_info_event())
		evt.info.set(winfo.c_str());

	const HRESULT rc = Send(evt);
	if (rc != S_OK) {		
		std::wstring desc{ dsFile_Reader };
		desc += dsFailed_To_Send_Event;
		desc += Describe_Error(rc);
		Emit_Info(scgms::NDevice_Event_Code::Error, desc);
	}

	return rc == S_OK;
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

void CFile_Reader::Run_Reader() {
	ExtractionResult values;	
	if (Extract(values) == S_OK) {

		Merge_Values(values);

		bool isError = false;
		uint32_t currentSegmentId = 0;

		double nextPhysicalActivityEnd = -1; // negative value = invalid
		double nextTempBasalEnd = -1;
		double nextSleepEnd = -1;
		double lastBasalRateSetting = 0.0;

		// for each file loaded...
		for (const auto& vals : mMergedValues)
		{
			std::list<TSegment_Limits> resolvedSegments;
			Resolve_Segments(vals, resolvedSegments);

			// for every segment resolved...
			for (const TSegment_Limits& seg : resolvedSegments)
			{
				currentSegmentId++;
				Send_Event(scgms::NDevice_Event_Code::Time_Segment_Start, vals[seg.first]->mMeasuredAt, currentSegmentId);

				for (size_t i = seg.first; i < seg.second; i++)
				{
					const CMeasured_Value* cur = vals[i];

					const double valDate = cur->mMeasuredAt;

					bool errorRes = false;

					if (nextPhysicalActivityEnd > 0 && nextPhysicalActivityEnd <= valDate)
					{
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, nextPhysicalActivityEnd, currentSegmentId, scgms::signal_Physical_Activity, 0);
						nextPhysicalActivityEnd = -1;
					}
					if (nextTempBasalEnd > 0 && nextTempBasalEnd <= valDate)
					{
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, nextTempBasalEnd, currentSegmentId, scgms::signal_Requested_Insulin_Basal_Rate, lastBasalRateSetting);
						nextTempBasalEnd = -1;
					}
					if (nextSleepEnd > 0 && nextSleepEnd < valDate)
					{
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, nextSleepEnd, currentSegmentId, scgms::signal_Sleep_Quality, 0);
						nextSleepEnd = -1;
					}

					// now go through every field, check for value presence, and send it to chain
					// NOTE: the extractor should be reworked to suit the new architecture; extractor code is not compatible by design
					//       this bridging code would then be discarded in favor of new, clean code

					if (cur->mIst.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_IG, cur->mIst.value());
					if (cur->mIsig.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_ISIG, cur->mIsig.value());
					if (cur->mBlood.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_BG, cur->mBlood.value());
					if (cur->mInsulinBolus.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Requested_Insulin_Bolus, cur->mInsulinBolus.value());

					if (cur->mInsulinTempBasalRate.has_value() && cur->mInsulinTempBasalRateEnd.has_value())
					{
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Requested_Insulin_Basal_Rate, cur->mInsulinTempBasalRate.value());
						nextTempBasalEnd = cur->mInsulinTempBasalRateEnd.value();
					}

					if (cur->mInsulinBasalRate.has_value())
					{
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Requested_Insulin_Basal_Rate, cur->mInsulinBasalRate.value());
						// cancel all temp basal settings - a new basal rate settings immediatelly overrides all temp basal rates
						lastBasalRateSetting = cur->mInsulinBasalRate.value();
						nextTempBasalEnd = -1;
					}
					if (cur->mCarbohydrates.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Carb_Intake, cur->mCarbohydrates.value());
					if (cur->mCalibration.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Calibration, cur->mCalibration.value());

					if (cur->mElectrodermalActivity.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Electrodermal_Activity, cur->mElectrodermalActivity.value());
					if (cur->mHeartrate.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Heartbeat, cur->mHeartrate.value());
					if (cur->mPhysicalActivity.has_value() && cur->mPhysicalActivityDuration.has_value())
					{
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Physical_Activity, cur->mPhysicalActivity.value());
						// physical activity gets "cancelled" after given duration
						nextPhysicalActivityEnd = valDate + cur->mPhysicalActivityDuration.value();
					}
					if (cur->mSkinTemperature.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Skin_Temperature, cur->mSkinTemperature.value());
					if (cur->mAirTemperature.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Air_Temperature, cur->mAirTemperature.value());
					if (cur->mSteps.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Steps, cur->mSteps.value());
					if (cur->mSleepQuality.has_value() && cur->mSleepEnd.has_value())
					{
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, valDate, currentSegmentId, scgms::signal_Sleep_Quality, cur->mSleepQuality.value());
						nextSleepEnd = cur->mSleepEnd.value();
					}
					if (cur->mStringNote.has_value())
						errorRes |= !Send_Event(scgms::NDevice_Event_Code::Information, valDate, currentSegmentId, Invalid_GUID, std::numeric_limits<double>::infinity(), Widen_Char(cur->mStringNote.value().c_str()));

					if (errorRes)
					{
						isError = true;
						break;
					}
				}

				if (!Send_Event(scgms::NDevice_Event_Code::Time_Segment_Stop, vals[seg.second - 1]->mMeasuredAt, currentSegmentId))
					isError = true;

				if (isError)
					break;
			}

			if (isError)
				break;
		}
	} else {
		 Emit_Info(scgms::NDevice_Event_Code::Error, dsUnexpected_Error_While_Parsing + mFileName.wstring());
	}

	if (mShutdownAfterLast)	{
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Shut_Down };

		evt.device_id() = file_reader::File_Reader_Device_GUID;
		Send(evt);
	}
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

	std::vector<filesystem::path> files_to_extract;

	if (Is_Directory(mFileName))
	{
		for (auto& path : filesystem::directory_iterator(mFileName))
		{
			if (Is_Regular_File_Or_Symlink(path))
				files_to_extract.push_back(path);
		}
	}
	else
		files_to_extract.push_back(mFileName);

	values.SegmentValues.resize(files_to_extract.size());
	values.SegmentBloodValues.resize(files_to_extract.size());
	values.SegmentMiscValues.resize(files_to_extract.size());

	for (size_t fileIndex = 0; fileIndex < files_to_extract.size(); fileIndex++)
	{
		CFormat_Adapter sfile;
		std::string format = recognizer.Recognize_And_Open(files_to_extract[fileIndex], sfile);

		// unrecognized format or error loading file
		if (format == "" || sfile.Get_Error())
		{
			Emit_Info(scgms::NDevice_Event_Code::Error, L"Unknown format: " + files_to_extract[fileIndex].wstring());
			return E_FAIL;
		}

		// extract data and fill extraction result
		if (!extractor.Extract(format, sfile, values, fileIndex))
		{
			Emit_Info(scgms::NDevice_Event_Code::Error, L"Cannot extract: " + files_to_extract[fileIndex].wstring());
			return E_FAIL;
		}
	}

	return S_OK;
}


HRESULT IfaceCalling CFile_Reader::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	HRESULT rc = E_UNEXPECTED;

	mFileName = configuration.Read_File_Path(rsInput_Values_File);
	mSegmentSpacing = configuration.Read_Int(rsInput_Segment_Spacing) * 1000.0 * InvMSecsPerDay;
	if (mSegmentSpacing == 0.0) mSegmentSpacing = file_reader::Default_Segment_Spacing;	//likely default, misconfigured value

	mShutdownAfterLast = configuration.Read_Bool(rsShutdown_After_Last);
	mMinValueCount = static_cast<size_t>(configuration.Read_Int(rsMinimum_Segment_Levels));
	mRequireBG_IG = configuration.Read_Bool(rsRequire_IG_BG);

	std::error_code ec;
	if ((Is_Regular_File_Or_Symlink(mFileName) || Is_Directory(mFileName)) && filesystem::exists(mFileName, ec)) {
		mReaderThread = std::make_unique<std::thread>(&CFile_Reader::Run_Reader, this);
		rc = S_OK;
	} else {		
		error_description.push(dsCannot_Open_File + mFileName.wstring());
		rc = E_NOTIMPL;
	}
	
	return rc;
}

HRESULT IfaceCalling CFile_Reader::Do_Execute(scgms::UDevice_Event event) {	
	return Send(event);
}