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
#include <cmath>
#include <set>

namespace file_reader
{
	const GUID File_Reader_Device_GUID = { 0x78df0982, 0x705, 0x4c84, { 0xb0, 0xa3, 0xae, 0x40, 0xfe, 0x7e, 0x18, 0x7b } };	// {78DF0982-0705-4C84-B0A3-AE40FE7E187B}

	// default value spacing in segments - this space determines when the segment ends and starts new
	//constexpr double Default_Segment_Spacing = 600.0 * 1000.0 * InvMSecsPerDay; -- unused as there's no way to expose default config values so far
}

CFile_Reader::CFile_Reader(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CFile_Reader::~CFile_Reader() {

	if (mReaderThread && mReaderThread->joinable())
		mReaderThread->join();	
}

bool CFile_Reader::Send_Event(scgms::NDevice_Event_Code code, double device_time, uint64_t segment_id, const GUID& signalId, const double value, const std::wstring& winfo)
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

	const HRESULT rc = mOutput.Send(evt);
	const bool succcess = Succeeded(rc);

	if (!succcess) {
		std::wstring desc{ dsFile_Reader };
		desc += dsFailed_To_Send_Event;
		desc += Describe_Error(rc);
		Emit_Info(scgms::NDevice_Event_Code::Error, desc);
	}

	return succcess;
}


void CFile_Reader::Resolve_Segments(TValue_Vector const& src, std::list<TSegment_Limits>& segment_start_stop) const {
	size_t segment_begin = 0;

	while (segment_begin < src.size()) {
		size_t segment_end = src.size() - 1;
		size_t ig_counter = 0;
		bool bg_is_present = false;
		double recent_ig_time = std::numeric_limits<double>::quiet_NaN();
		
		for (size_t current = segment_begin; current < src.size(); current++) {
			
			const auto& current_levels = src[current];
			const double current_measured_time = current_levels.measured_at();

			//if recent_ig_time is nan, then we consume any value as we wait for the first IG
			if (!std::isnan(current_measured_time)) {
				const bool is_in_allowed_interval = std::isnan(recent_ig_time) || (recent_ig_time + mMaximum_IG_Interval >= current_measured_time);

				if (is_in_allowed_interval) {
					const auto ig_val = current_levels.get<double>(scgms::signal_IG);

					if (ig_val) {
						//we've got IG level so that the segment looks OK
						ig_counter++;
						recent_ig_time = current_measured_time;
					}

					//do not forget that src[current] may hold more than one signal
					//this level still falls into the maximum allowed interval between consecutive IGs
					if (current_levels.has_value(scgms::signal_BG) || current_levels.has_value(scgms::signal_BG_Calibration))
						bg_is_present = true;
				}
				else {
					//this level has exceeded the maximum allowed interval between consecutive IGs
					//=>push it as the segment's end

					segment_end = current - 1; //move one back as this level belongs to the following segment
					break;
				}
			}
		}


		segment_end++;
		
		if ((ig_counter >= mMinimum_Required_IGs) &&					//minimum number of IG levels met
			(!mRequire_BG || (mRequire_BG && bg_is_present)))		//BG is present, if required
			segment_start_stop.push_back({ segment_begin, segment_end});

		//ig_counter = 0;
		//bg_is_present = false;
		segment_begin = segment_end;			//at this level, new segment's start
		//recent_ig_time = std::numeric_limits<double>::quiet_NaN();
	}
}

void CFile_Reader::Run_Reader() {
	ExtractionResult values;	
	if (Extract(values) == S_OK) {

		Merge_Values(values);

		bool isError = false;
		uint32_t currentSegmentId = 0;

		double nextPhysicalActivityEnd = -1.0; // negative value = invalid
		double nextTempBasalEnd = -1.0;
		double nextSleepEnd = -1.0;
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
				Send_Event(scgms::NDevice_Event_Code::Time_Segment_Start, vals[seg.first].measured_at(), currentSegmentId);

				for (size_t i = seg.first; i < seg.second; i++) {

					const auto& current_values = vals[i];
					if (!current_values.valid())
						continue;
					const double current_measured_time = current_values.measured_at();
					
					bool errorRes = false;

					current_values.enumerate([=, &errorRes, &nextPhysicalActivityEnd, &nextTempBasalEnd, &lastBasalRateSetting, &nextSleepEnd](const GUID& signal_id, const CMeasured_Values_At_Single_Time::TValue& val) {
						const std::set<GUID> silenced_signals = { signal_Physical_Activity_Duration, signal_Insulin_Temp_Rate, signal_Insulin_Temp_Rate_Endtime, signal_Sleep_Endtime, signal_Comment };

						const double* current_level = std::get_if<double>(&val);

						//handle the known, special cases which use helper signals 					
						if (signal_id == signal_Physical_Activity_Duration) {
							if (current_level != nullptr)
								nextPhysicalActivityEnd = current_measured_time + *current_level;
							//note that we do not set the activity itself here, because it will be done in the generic part of the code
						}

						else if (signal_id == signal_Insulin_Temp_Rate_Endtime) {
							//note that we drive the temp by the end-time, becuase the temp basal must have the temp time - otherwise, it won't be a temporarily change
							if (current_level != nullptr) {
								//check the temp basal  rate
								const auto new_temp_basal = current_values.get<double>(signal_Insulin_Temp_Rate);
								if (new_temp_basal) {

									errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, current_measured_time, currentSegmentId, scgms::signal_Requested_Insulin_Basal_Rate, new_temp_basal.value());
									nextTempBasalEnd = *current_level;
								}
							}						
						}

						else if (signal_id == scgms::signal_Requested_Insulin_Basal_Rate) {
							if (current_level != nullptr) {
								lastBasalRateSetting = *current_level;
								// cancel all temp basal settings - a new basal rate settings immediatelly overrides all temp basal rates
								nextTempBasalEnd = -1.0;

								//note that the new basal rate will be sent in the generic part of the code
							}						
						}
						
						else if (signal_id == signal_Sleep_Endtime) {
							//just like with the temp basal, we require the sleep end time to prevent the patient falling asleep forever
							if (current_level != nullptr) {
								const auto sleep_quality = current_values.get<double>(scgms::signal_Sleep_Quality);
								if (sleep_quality) {
									errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, current_measured_time, currentSegmentId, scgms::signal_Sleep_Quality, *current_level);
									nextSleepEnd = *current_level;	//yes, this is given in absolute time
								}
							}
						}


						else if (signal_id == signal_Comment) {
							const std::string *comment = std::get_if<std::string>(&val);
							if (comment != nullptr) {
								std::wstring wstr = Widen_String(*comment);
								errorRes |= !Send_Event(scgms::NDevice_Event_Code::Information, current_measured_time, currentSegmentId, Invalid_GUID, std::numeric_limits<double>::infinity(), wstr);
							}
						}

						
						//send all the non-helper signals, which we do not suppress
						if (silenced_signals.find(signal_id) == silenced_signals.end()) {												
							if (current_level != nullptr)
								errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, current_measured_time, currentSegmentId, signal_id, *current_level);
						}					



						//switch off the physical activity, once ended
						if ((nextPhysicalActivityEnd > 0.0) && (nextPhysicalActivityEnd <= current_measured_time)) {
							errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, nextPhysicalActivityEnd, currentSegmentId, scgms::signal_Physical_Activity, 0.0);
							nextPhysicalActivityEnd = -1.0;
						}

						//restore the insulin basal rate settings if the basal temp has ended
						if ((nextTempBasalEnd > 0.0) && (nextTempBasalEnd <= current_measured_time)) {
							errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, nextTempBasalEnd, currentSegmentId, scgms::signal_Requested_Insulin_Basal_Rate, lastBasalRateSetting);
							nextTempBasalEnd = -1.0;
						}


						//wake up the patient
						if ((nextSleepEnd > 0.0) && (nextSleepEnd < current_measured_time))
						{
							errorRes |= !Send_Event(scgms::NDevice_Event_Code::Level, nextSleepEnd, currentSegmentId, scgms::signal_Sleep_Quality, 0.0);
							nextSleepEnd = -1.0;
						}
					});


					if (errorRes)
					{
						isError = true;
						break;
					}
				}

				if (!Send_Event(scgms::NDevice_Event_Code::Time_Segment_Stop, vals[seg.second - 1].measured_at(), currentSegmentId))
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
		
		const auto& last_seg = mMergedValues[mMergedValues.size() - 1];
		evt.device_time() = last_seg[last_seg.size() - 1].measured_at() + std::numeric_limits<double>::epsilon();
		
		evt.device_id() = file_reader::File_Reader_Device_GUID;
		mOutput.Send(evt);
	}
}

void CFile_Reader::Merge_Values(ExtractionResult& result)
{
	// prepare output vector
	mMergedValues.resize(result.mValues.size());	//resize to the number of segments

	// go through all loaded values and push them to single vector containing all values	
	for (size_t i = 0; i < result.mValues.size(); i++) {
		for (size_t j = 0; j < result.mValues[i].size(); j++)
			mMergedValues[i].push_back(result.mValues[i][j]);

		// sort values by timestamp from ascending
		std::sort(mMergedValues[i].begin(), mMergedValues[i].end(), [](CMeasured_Values_At_Single_Time &a, CMeasured_Values_At_Single_Time &b) { return a.measured_at() < b.measured_at(); });
	}
}

HRESULT CFile_Reader::Extract(ExtractionResult &values)
{
	CFormat_Recognizer recognizer;
	CExtractor extractor;
	CDateTime_Recognizer dt_formats;

	CFormat_Rule_Loader loader(recognizer, extractor, dt_formats);
	if (!loader.Load())
		return E_FAIL;


	values.Value_Count = 0;
	values.mValues.clear();

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

	values.mValues.resize(files_to_extract.size());

	for (size_t fileIndex = 0; fileIndex < files_to_extract.size(); fileIndex++) {
		CFormat_Adapter sfile;
		std::string format = recognizer.Recognize_And_Open(files_to_extract[fileIndex], sfile);

		// unrecognized format or error loading file
		if (format == "" || sfile.Get_Error())
		{
			Emit_Info(scgms::NDevice_Event_Code::Error, L"Unknown format: " + files_to_extract[fileIndex].wstring());
			return E_FAIL;
		}

		// extract data and fill extraction result
		if (!extractor.Extract(format, sfile, dt_formats, values, fileIndex))
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
	mMaximum_IG_Interval = configuration.Read_Double(rsMaximum_IG_Interval, mMaximum_IG_Interval);
	if (std::isnan(mMaximum_IG_Interval) || (mMaximum_IG_Interval <= 0.0)) {
		error_description.push(L"Maximum IG interval must be a positive number. 12 minutes is a default value.");
		return E_INVALIDARG;
	}

	mShutdownAfterLast = configuration.Read_Bool(rsShutdown_After_Last);
	mMinimum_Required_IGs = static_cast<size_t>(configuration.Read_Int(rsMinimum_Required_IGs));
	mRequire_BG = configuration.Read_Bool(rsRequire_BG);

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
	return mOutput.Send(event);
}
