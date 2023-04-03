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

#include "healthkit_dump_reader.h"
#include "descriptor.h"

#include <iostream>
#include <set>
#include <string>

#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/XMLParser.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/utils/DebugHelper.h"
#include "../../../common/rtl/rattime.h"

namespace healthkit_dump_reader_filter
{
	//
}

/*
HealthKit known quantity/category names:

HKQuantityTypeIdentifierDietaryCarbohydrates
HKQuantityTypeIdentifierInsulinDelivery
HKQuantityTypeIdentifierHeartRate
HKQuantityTypeIdentifierRestingHeartRate
HKQuantityTypeIdentifierWalkingSpeed
HKQuantityTypeIdentifierStepCount
HKCategoryTypeIdentifierAppleStandHour
HKCategoryTypeIdentifierMenstrualFlow
HKCategoryTypeIdentifierMindfulSession
HKCategoryTypeIdentifierSexualActivity
HKQuantityTypeIdentifierActiveEnergyBurned
HKQuantityTypeIdentifierAppleExerciseTime
HKQuantityTypeIdentifierAppleStandTime
HKQuantityTypeIdentifierBasalEnergyBurned
HKQuantityTypeIdentifierBodyMass
HKQuantityTypeIdentifierDistanceCycling
HKQuantityTypeIdentifierDistanceWalkingRunning
HKQuantityTypeIdentifierFlightsClimbed
HKQuantityTypeIdentifierHeadphoneAudioExposure
HKQuantityTypeIdentifierHeartRateVariabilitySDNN
HKQuantityTypeIdentifierHeight
HKQuantityTypeIdentifierVO2Max
HKQuantityTypeIdentifierWalkingAsymmetryPercentage
HKQuantityTypeIdentifierWalkingDoubleSupportPercentage
HKQuantityTypeIdentifierWalkingHeartRateAverage
HKQuantityTypeIdentifierWalkingStepLength

*/


CHealthKit_Dump_Reader::CHealthKit_Dump_Reader(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CHealthKit_Dump_Reader::~CHealthKit_Dump_Reader() {
	//
}

HRESULT IfaceCalling CHealthKit_Dump_Reader::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<scgms::IFilter>(healthkit_dump_reader_filter::id, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

HRESULT IfaceCalling CHealthKit_Dump_Reader::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	try
	{
		auto dateToRat = [](const std::wstring& str) -> double {
			return Local_Time_WStr_To_Rat_Time(str, L"%Y-%m-%d %H:%M:%S");
		};

		CXML_Parser<wchar_t> parser(configuration.Read_File_Path(rsFile_Path));

		const auto& el = parser.Get_Element(L"HealthData");
		const auto& recs = el.children.find(L"Record");
		if (recs != el.children.end())
		{
			const auto& recs_v = recs->second;
			for (auto& rec : recs_v)
			{
				auto str = rec.Get_Parameter(L"type");
				if (str == L"HKQuantityTypeIdentifierBloodGlucose")
				{
					auto val = std::stod(rec.Get_Parameter(L"value"));
					auto date = dateToRat(rec.Get_Parameter(L"startDate"));

					mSignals.push_back({ scgms::signal_IG, date, val });
				}
				else if (str == L"HKQuantityTypeIdentifierInsulinDelivery")
				{
					auto val = std::stod(rec.Get_Parameter(L"value"));
					auto date = dateToRat(rec.Get_Parameter(L"startDate"));

					const auto& typeEl = rec.children.find(L"MetadataEntry");
					if (typeEl != rec.children.end())
					{
						for (const auto& tel : typeEl->second)
						{
							const auto& key = tel.Get_Parameter(L"key");

							if (key == L"HKInsulinDeliveryReason")
							{
								auto cval = tel.Get_Parameter(L"value");
								if (cval == L"1")
									mSignals.push_back({ scgms::signal_Requested_Insulin_Basal_Rate, date, val });
								else if (cval == L"2")
									mSignals.push_back({ scgms::signal_Requested_Insulin_Bolus, date, val });
							}
						}
					}
				}
				else if (str == L"HKQuantityTypeIdentifierDietaryCarbohydrates")
				{
					auto val = std::stod(rec.Get_Parameter(L"value"));
					auto date = dateToRat(rec.Get_Parameter(L"startDate"));

					mSignals.push_back({ scgms::signal_Carb_Intake, date, val });
				}
				else if (str == L"HKQuantityTypeIdentifierHeartRate" || str == L"HKQuantityTypeIdentifierRestingHeartRate" /* TODO: distinguish somehow? */)
				{
					auto val = std::stod(rec.Get_Parameter(L"value"));
					auto date = dateToRat(rec.Get_Parameter(L"startDate"));

					mSignals.push_back({ scgms::signal_Heartbeat, date, val });
				}
				else if (str == L"HKQuantityTypeIdentifierWalkingSpeed")
				{
					auto val = std::stod(rec.Get_Parameter(L"value"));
					auto date = dateToRat(rec.Get_Parameter(L"startDate"));

					mSignals.push_back({ scgms::signal_Movement_Speed, date, val });
				}
				else if (str == L"HKQuantityTypeIdentifierStepCount")
				{
					auto val = std::stod(rec.Get_Parameter(L"value"));
					auto date = dateToRat(rec.Get_Parameter(L"startDate"));

					mSignals.push_back({ scgms::signal_Steps, date, val });
				}
			}
		}
	}
	catch (std::exception& what)
	{
		dprintf( what.what() );
	}

	uint64_t segId = 0;
	double lastValueTime = 0;

	auto endSegment = [&](double time, uint64_t segmentId) {
		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Time_Segment_Stop };
		evt.device_time() = time;
		evt.signal_id() = Invalid_GUID;
		evt.segment_id() = segmentId;
		evt.device_id() = Invalid_GUID;
		mOutput.Send(evt);
	};

	auto startNewSegment = [&](double time) {

		if (segId > 0)
			endSegment(lastValueTime, segId);

		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Time_Segment_Start };
		evt.device_time() = time;
		evt.signal_id() = Invalid_GUID;
		evt.segment_id() = ++segId;
		evt.device_id() = Invalid_GUID;
		mOutput.Send(evt);
	};

	std::sort(mSignals.begin(), mSignals.end(), [](const SignalEntry& a, const SignalEntry& b) { return a.time < b.time; });

	std::vector<std::pair<size_t, size_t>> segments;
	segments.resize(1);
	size_t lastEnd = 0;
	bool hasIG = false;
	for (size_t idx = 0; idx < mSignals.size(); idx++)
	{
		const auto& sg = mSignals[idx];

		if (lastValueTime > 0 && sg.time - lastValueTime > scgms::One_Minute * 20)
		{
			if (idx - lastEnd > 10 && hasIG)
				segments.push_back({ lastEnd, idx });
			lastEnd = idx;
			hasIG = false;
		}

		lastValueTime = sg.time;
		if (sg.id == scgms::signal_IG)
			hasIG = true;
	}

	for (const auto& segment : segments)
	{
		startNewSegment(mSignals[segment.first].time);

		for (size_t idx = segment.first; idx < segment.second; idx++)
		{
			const auto& sg = mSignals[idx];

			lastValueTime = sg.time;

			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
			evt.device_time() = sg.time;
			evt.signal_id() = sg.id;
			evt.level() = sg.value;
			evt.segment_id() = segId;
			evt.device_id() = Invalid_GUID;
			mOutput.Send(evt);
		}
	}

	endSegment(lastValueTime, segId);

	return S_OK;
}

HRESULT IfaceCalling CHealthKit_Dump_Reader::Do_Execute(scgms::UDevice_Event event) {
		
	return mOutput.Send(event);
}
