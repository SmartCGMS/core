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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "oref_settings.h"
#include "descriptor.h"

#include <sstream>

const char* rsPlaceholder_Delta = "%%delta%%";
const char* rsPlaceholder_GlucStatusDate = "%%gsdate%%";
const char* rsPlaceholder_LongAvgDelta = "%%longavgdelta%%";
const char* rsPlaceholder_ShortAvgDelta = "%%shortavgdelta%%";
const char* rsPlaceholder_CurGlucose = "%%glucose%%";
const char* rsPlaceholder_ParamISF = "%%isf%%";
const char* rsPlaceholder_ParamCarbRatio = "%%carb_ratio%%"; // 1/csr
const char* rsPlaceholder_IOB = "%%iob%%";
const char* rsPlaceholder_IOB_Data = "%%iob_data%%";
const char* rsPlaceholder_InsulinActivity = "%%insactivity%%";
const char* rsPlaceholder_CurrentTempRate = "%%curtemprate%%";
const char* rsPlaceholder_CurrentTempDuration = "%%curtempduration%%";
const char* rsPlaceholder_Carbs = "%%carbs%%";
const char* rsPlaceholder_LastCarbTime = "%%lcarbtime%%";
const char* rsPlaceholder_COB = "%%cob%%";
const char* rsPlaceholder_CurBasal = "%%curbasal%%";
const char* rsPlaceholder_LastTemp_Date = "%%lasttemp_date%%";
const char* rsPlaceholder_LastTemp_Rate = "%%lasttemp_rate%%";
const char* rsPlaceholder_LastBolus_Date = "%%lastbolustime%%";
const char* rsPlaceholder_LastBolus_Value = "%%lastbolusvalue%%";
const char* rsPlaceholder_CurDate = "%%curdate%%";

const char* rsPlaceholder_Oref1Enable = "%%oref1_enable%%";
const char* rsPlaceholder_Autosens_Min = "%%autosens_min%%";
const char* rsPlaceholder_Autosens_Max = "%%autosens_max%%";

#define AUTOSENS_MIN "0.7"
#define AUTOSENS_MAX "1.2"

#define AUTOSENS_ENABLE "true"
#define OREF1_ENABLE "true"

const char* rsIOBDataElement = R"SCRIPT({ "iob": %%iob%%, "activity": %%insactivity%%, "bolussnooze": 0, "iobWithZeroTemp": {"activity": 0}, "lastTemp": { "date": %%lasttemp_date%%, "rate": %%lasttemp_rate%% }, "lastBolusTime": %%lastbolustime%% })SCRIPT";

const char* rsBaseProgram = 
R"SCRIPT(var glucose_status = {
	"date": %%gsdate%%,
	"delta": %%delta%%,
	"glucose": %%glucose%%,
	"long_avgdelta": %%longavgdelta%%,
	"short_avgdelta": %%shortavgdelta%%
};

var currenttemp = {
	"duration": %%curtempduration%%,
	"rate": %%curtemprate%%,
	"temp": "absolute"
};

var iob_data = [ %%iob_data%% ];

var profile = {
	"max_iob": 12.0,
	"type": "current",
	"current_basal": %%curbasal%%,
	"max_daily_basal": 150.0,
	"max_basal": 3.5,
	"max_bg": 140,
	"min_bg": 100,
	"sens": %%isf%%,
	"carb_ratio": %%carb_ratio%%,
	"current_date": %%curdate%%,
	"A52_risk_enable": false,
	"carbsReqThreshold": 1,
	"maxCOB": 120,
	"enableSMB_always": %%oref1_enable%%,
	"enableSMB_with_COB": %%oref1_enable%%,
	"enableSMB_with_temptarget": %%oref1_enable%%,
	"allowSMB_with_high_temptarget": %%oref1_enable%%,
	"enableSMB_after_carbs": %%oref1_enable%%,
	"enableUAM": %%oref1_enable%%,
	"autosens_max": %%autosens_max%%,
	"autosens_min": %%autosens_min%%
};

var meal_data = {
	"carbs": %%carbs%%,
	"lastCarbTime": %%lcarbtime%%,
	"mealCOB": %%cob%%,
	"lastBolusTime": %%lastbolustime%%,
	"boluses": %%lastbolusvalue%%
};

var output = determine_basal(glucose_status, currenttemp, iob_data, profile, undefined, meal_data, tempBasalFunctions, %%oref1_enable%%);

var rate = (typeof output.rate === "undefined" ? 0 : output.rate);
var smbunits = (typeof output.units === "undefined" ? 0 : output.units);

[ rate, smbunits ];)SCRIPT";

namespace oref_utils
{
	void Substr_Replace(std::string& str, const std::string& what, const std::string& replace)
	{
		size_t pos;
		while ((pos = str.find(what)) != std::string::npos)
			str = str.replace(str.begin() + pos, str.begin() + (pos + what.length()), replace);
	}

	std::string Prepare_Base_Program(const COref_Instance_Data& data, const oref_model::TParameters& parameters)
	{
		std::string program = rsBaseProgram;

		Substr_Replace(program, rsPlaceholder_Oref1Enable, OREF1_ENABLE);
		Substr_Replace(program, rsPlaceholder_Autosens_Min, AUTOSENS_MIN);
		Substr_Replace(program, rsPlaceholder_Autosens_Max, AUTOSENS_MAX);

		Substr_Replace(program, rsPlaceholder_Delta, std::to_string(data.mDelta));
		Substr_Replace(program, rsPlaceholder_LongAvgDelta, std::to_string(data.mLongAvgDelta));
		Substr_Replace(program, rsPlaceholder_ShortAvgDelta, std::to_string(data.mShortAvgDelta));
		Substr_Replace(program, rsPlaceholder_CurGlucose, std::to_string(data.mGlucose));

		Substr_Replace(program, rsPlaceholder_CurDate, std::to_string(data.mCurTime));
		Substr_Replace(program, rsPlaceholder_GlucStatusDate, std::to_string(data.mCurTime));

		Substr_Replace(program, rsPlaceholder_ParamISF, std::to_string((1.0 / parameters.isf)*18.018)); // ISF = how much insulin [U] to lower BG by 1mmol/l; oref0 interpretation = 1 unit of insulin lowers BG by this much mg/dL
		Substr_Replace(program, rsPlaceholder_ParamCarbRatio, std::to_string(1.0 / parameters.cr));

		bool frst = true;
		std::stringstream iobss;
		for (size_t i = 0; i < data.mCurIOB.size(); i++)
		{
			if (!std::isnan(data.mCurIOB[i]) && !std::isnan(data.mCurInsulinActivity[i]))
			{
				if (frst)
					frst = false;
				else
					iobss << ", ";

				std::string base = rsIOBDataElement;
				Substr_Replace(base, rsPlaceholder_IOB, std::to_string(data.mCurIOB[i]));
				Substr_Replace(base, rsPlaceholder_InsulinActivity, std::to_string(data.mCurInsulinActivity[i]));

				if (i == 0)
				{
					Substr_Replace(base, rsPlaceholder_LastTemp_Date, std::to_string(data.mLastTime * 1000));
					Substr_Replace(base, rsPlaceholder_LastTemp_Rate, std::to_string(data.mResultRate));

					if (data.mLastBolusTime > 0)
					{
						Substr_Replace(base, rsPlaceholder_LastBolus_Date, std::to_string(data.mLastBolusTime * 1000));
						Substr_Replace(base, rsPlaceholder_LastBolus_Value, std::to_string(data.mLastBolusValue));
					}
					else
					{
						Substr_Replace(base, rsPlaceholder_LastBolus_Date, std::to_string(data.mLastTime /* TODO: verify */ * 1000)); // to allow SMB to function, this time has to be more than 3 minutes in past
						Substr_Replace(base, rsPlaceholder_LastBolus_Value, std::to_string(0));
					}
				}
				else
				{
					Substr_Replace(base, rsPlaceholder_LastTemp_Date, "null");
					Substr_Replace(base, rsPlaceholder_LastTemp_Rate, "null");
					Substr_Replace(base, rsPlaceholder_LastBolus_Date, "null");
				}

				iobss << base;
			}
		}

		Substr_Replace(program, rsPlaceholder_IOB_Data, iobss.str());

		if (data.mLastCarbTime > 0)
		{
			Substr_Replace(program, rsPlaceholder_COB, std::to_string(data.mCurCOB));
			Substr_Replace(program, rsPlaceholder_LastCarbTime, std::to_string(data.mLastCarbTime * 1000)); // unfortunately, oref0 always calculates with current timestamp (simulation-unfriendly)
			Substr_Replace(program, rsPlaceholder_Carbs, std::to_string(data.mLastCarbValue));
		}
		else // otherwise carbs are zero
		{
			Substr_Replace(program, rsPlaceholder_COB, "0");
			Substr_Replace(program, rsPlaceholder_LastCarbTime, "0");
			Substr_Replace(program, rsPlaceholder_Carbs, "0");
		}

		Substr_Replace(program, rsPlaceholder_CurrentTempRate, std::to_string(data.mResultRate));
		Substr_Replace(program, rsPlaceholder_CurrentTempDuration, std::to_string(data.mResultDuration - (data.mCurTime - data.mLastTime))); // last tick was 5 minutes ago, oref0 requires "remaining time" here
		Substr_Replace(program, rsPlaceholder_CurBasal, std::to_string(parameters.ibr));

		if (data.mLastBolusTime > 0)
		{
			Substr_Replace(program, rsPlaceholder_LastBolus_Date, std::to_string(data.mLastBolusTime * 1000));
			Substr_Replace(program, rsPlaceholder_LastBolus_Value, std::to_string(data.mLastBolusValue));
		}
		else
		{
			Substr_Replace(program, rsPlaceholder_LastBolus_Date, std::to_string(data.mLastTime /* TODO: verify */ * 1000));
			Substr_Replace(program, rsPlaceholder_LastBolus_Value, std::to_string(0));
		}

		return program;
	}
}
