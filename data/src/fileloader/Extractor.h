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

#pragma once

#include <vector>
#include <map>
#include <locale>

#include "FormatAdapter.h"
#include "Structures.h"
#include "time_utils.h"
#include "FormatImpl.h"

// enumerator of recognized columns
enum class ExtractorColumns
{
	NONE,
	COL_DATE,
	COL_TIME,
	COL_DATETIME,
	COL_ISIG,
	COL_IST,
	COL_BLOOD,
	COL_BLOOD_DATE,
	COL_BLOOD_TIME,
	COL_BLOOD_DATETIME,
	COL_BLOOD_CALIBRATION,
	COL_DEVICE,
	COL_INSULIN_BOLUS,
	COL_INSULIN_BOLUS_DATETIME,
	COL_INSULIN_BASAL_RATE,
	COL_INSULIN_BASAL_RATE_DATETIME,
	COL_INSULIN_TEMP_BASAL_RATE,
	COL_INSULIN_TEMP_BASAL_RATE_DATETIME_BEGIN,
	COL_INSULIN_TEMP_BASAL_RATE_DATETIME_END,
	COL_CARBOHYDRATES,
	COL_CARBOHYDRATES_DATETIME,
	COL_EVENT,
	COL_EVENT_DATE,
	COL_EVENT_TIME,
	COL_EVENT_DATETIME,
	COL_EVENT_CONDITION,
	COL_PHYSICAL_ACTIVITY,
	COL_PHYSICAL_ACTIVITY_DURATION,
	COL_PHYSICAL_ACTIVITY_DATETIME,
	COL_SKIN_TEMPERATURE,
	COL_SKIN_TEMPERATURE_DATETIME,
	COL_AIR_TEMPERATURE,
	COL_AIR_TEMPERATURE_DATETIME,
	COL_HEARTRATE,
	COL_HEARTRATE_DATETIME,
	COL_ELECTRODERMAL_ACTIVITY,
	COL_ELECTRODERMAL_ACTIVITY_DATETIME,
	COL_STEPS,
	COL_STEPS_DATETIME,
	COL_SLEEP,
	COL_SLEEP_DATETIME,
	COL_SLEEP_DATETIME_END,
	COL_ACCELERATION_MAGNITUDE,
	COL_ACCELERATION_MAGNITUDE_DATETIME,
	COL_MISC_NOTE,
};

// type of iteration during extraction
enum class ExtractionIterationType
{
	IST,
	BLOOD,
	EVENT,
	ADDITIONAL,
};


/*
 * Structure used as result of extraction
 */
struct TExtracted_Series {

	std::vector<CMeasured_Levels> mValues;

	/*TODO: remove
	// ordered measured values of files, initially IST-only, then used for merging blood glucose levels; primary index: file index, secondary index: value position
	std::vector<std::vector<CMeasured_Value*>> SegmentValues;
	// ordered measured BG values of files; primary index: file index, secondary index: value position
	std::vector<std::vector<CMeasured_Value*>> SegmentBloodValues;
	// ordered measured values of miscellanous quantities; primary index: file index, secondary index: value position
	std::vector<std::vector<CMeasured_Value*>> SegmentMiscValues;
	*/
	// segment file index assigned
	std::vector<size_t> SegmentFileIndex;
	// recognized device for each file; index: file index, value: device name; empty string if not recognized
	std::vector<std::string> FileDeviceNames;

	// total value count within all segments
	size_t Value_Count =  0;
};

CMeasured_Levels Extract_From_File(CFormat_Adapter& source);
