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

#include <iostream>

#include "dmms_lib.h"
#include "../../../../common/rtl/rattime.h"
#include "../../../../common/lang/dstrings.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/FilesystemLib.h"
#include "../../../../common/utils/XMLParser.h"
#include "../../../../common/rtl/FilesystemLib.h"
#include "../../../../common/utils/string_utils.h"

std::unique_ptr<CDynamic_Library> CDMMS_Lib_Discrete_Model::mDmmsLib;

std::array<char*, 0> CDMMS_Lib_Discrete_Model::mDmms_Out_Signal_Names = { };

CDMMS_Lib_Discrete_Model::CDMMS_Lib_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output)
	: CBase_Filter(output),
	  mParameters(scgms::Convert_Parameters<dmms_model::TParameters>(parameters, dmms_model::default_parameters.vector))
{
	//
}

CDMMS_Lib_Discrete_Model::~CDMMS_Lib_Discrete_Model()
{
	if (mDmmsSimThread && mDmmsSimThread->joinable())
		mDmmsSimThread->join();

	if (mDmmsEngine)
		mDmmsEngine.reset(nullptr);

	// do not unload DMMS library with each destructor; we keep the DLL loaded
	//if (mDmmsLib && mDmmsLib->Is_Loaded())
	//	mDmmsLib->Unload();
}

HRESULT CDMMS_Lib_Discrete_Model::Do_Execute(scgms::UDevice_Event event)
{
	HRESULT rc = S_FALSE;

	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate)
		{
			mToDMMS.basal_rate = event.level();
			rc = S_OK;
		}
		else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus)
		{
			// DMMS expects bolus injections in pmol/min, but we use "U/hr" - convert to "U/min" here, conversion to pmol/min is performed on DMMS handler side
			mToDMMS.bolus_rate += 60.0*event.level();
			rc = S_OK;
		}
		// TODO: carb rate
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		std::unique_lock<std::mutex> lck(mDmmsSimMtx);

		mRunning = false;
		mDmmsSimCv.notify_all();
	}

	if (rc == S_FALSE)
		rc = Send(event);

	return rc;
}

HRESULT CDMMS_Lib_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	std::filesystem::path root_path = Get_Application_Dir();

	auto path_to_absolute = [&root_path](const std::wstring& src_path)->std::wstring {
		std::filesystem::path path{ src_path };
		if (path.is_absolute()) return src_path;

		path = root_path / path;
		return std::filesystem::absolute(path);
	};


	std::filesystem::path mainfest_path = root_path / L"dmms_manifest.xml";

	CXML_Parser<wchar_t> parser(mainfest_path);
	if (!parser.Is_Valid())
		return false;

	mDMMS_Scenario_File = path_to_absolute(parser.Get_Parameter(L"manifest.dmms:scenario_file", L"dmms_scenario.xml"));
	mDMMS_Out_File = path_to_absolute(parser.Get_Parameter(L"manifest.dmms:log_file", L"dmms_out.txt"));
	mDMMS_Results_Dir = path_to_absolute(parser.Get_Parameter(L"manifest.dmms:result_dir", L"dmms_results"));

	if (!mDmmsLib || !mDmmsLib->Is_Loaded())
	{
		auto dllpath = Get_Application_Dir();
		Path_Append(dllpath, L"DmmsLib");

		// needed due to possibility of multiple dependency library versions coexisting in different fallback search paths
		// TODO: revisit this in future DMMS.dll releases
		//       last check: version 1.2.1
		SetDllDirectoryW(dllpath.c_str());

		Path_Append(dllpath, L"DMMS.dll");

		mDmmsLib = std::make_unique<CDynamic_Library>();

		if (!mDmmsLib->Load(dllpath.c_str()))
			return ENODEV;

		IDmmsFactory factory = mDmmsLib->Resolve<IDmmsFactory>("getDmmsEngine");
		if (!factory)
			return E_FAIL;

		mDmmsEngine.reset(factory());
	}

	mToSmartCGMS.insulin_injection = 0;
	mToDMMS.basal_rate = 0;
	mToDMMS.bolus_rate = 0;
	mToDMMS.carbs_rate = 0;
	mToDMMS.device_time = 0;
	mToDMMSValuesReady = true; // NOTE: this is set to true as we need the DMMS to emit initial values
	mToSmartCGMSValuesReady = false;

	mRunning = true;

	mDmmsSimThread = std::make_unique<std::thread>(&CDMMS_Lib_Discrete_Model::DMMS_Simulation_Thread_Fnc, this);

	return S_OK;
}

void CDMMS_Lib_Discrete_Model::DMMS_Simulation_Thread_Fnc()
{
	mDmmsEngine->runSim(Narrow_WString(mDMMS_Scenario_File).c_str(), Narrow_WString(mDMMS_Out_File).c_str(), Narrow_WString(mDMMS_Results_Dir).c_str(), this);

	mDmmsEngine->close();
	mDmmsEngine.reset(nullptr);
}

HRESULT IfaceCalling CDMMS_Lib_Discrete_Model::Step(const double time_advance_delta)
{
	if (!mRunning)
		return E_ILLEGAL_STATE_CHANGE;

	TDMMS_To_SmartCGMS values;

	// lock scope
	{
		std::unique_lock<std::mutex> lck(mDmmsSimMtx);

		mToDMMS.device_time = mLastTime + time_advance_delta;

		mToDMMSValuesReady = true;
		mDmmsSimCv.notify_all();

		mDmmsSimCv.wait(lck, [this]() { return mToSmartCGMSValuesReady || !mRunning; });

		// copy values before releasing the mDmmsSimMtx mutex, so we can use the shared filter mutex (in signal emit) without the risk of deadlock and loss of consistency
		values = mToSmartCGMS;
	}

	if (!mRunning)
		return S_FALSE; // terminated during wait

	mToSmartCGMSValuesReady = false;

	const double device_time = mTimeStart + scgms::One_Minute * values.simulation_time;

	if (std::isnan(mAnnounced.announcedMeal.time) && values.announced_meal.grams > 0 && values.announced_meal.time > 0)
	{
		/* meal_announce_offset - this is the time before actual meal (negative = before meal, positive = after meal)
		   we use 10 minutes before meal as default; study https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5836969/ suggests 15-20 minutes,
		   but in real scenario, it's often either exactly the time we start eating, a few minutes before that or
		   in worst case, several minutes after
		*/

		/* T1DMS performs some magic on announced meals; the original T1DMS controller checked for line-approximated
		   signal intersection with X axis and then calculated bolus; we have completely different logic, so no such
		   calculations are needed here */
		const double t1dms_compat = scgms::One_Minute * values.announced_meal.time;

		mAnnounced.announcedMeal.time = device_time + t1dms_compat + mParameters.meal_announce_offset;
		if (mAnnounced.announcedMeal.time <= mAnnounced.announcedMeal.lastTime)
			mAnnounced.announcedMeal.time = std::numeric_limits<double>::quiet_NaN();
		else
			mAnnounced.announcedMeal.grams = values.announced_meal.grams;
	}

	if (std::isnan(mAnnounced.announcedExcercise.time) && values.announced_excercise.intensity > 0 && values.announced_excercise.time > 0)
	{
		/* see note in announced meals */
		const double t1dms_compat = scgms::One_Minute * values.announced_excercise.time;

		mAnnounced.announcedExcercise.time = device_time + t1dms_compat + mParameters.excercise_announce_offset;
		mAnnounced.announcedExcercise.duration = scgms::One_Minute * values.announced_excercise.duration;
		if (mAnnounced.announcedExcercise.time <= mAnnounced.announcedExcercise.lastTime)
			mAnnounced.announcedExcercise.time = std::numeric_limits<double>::quiet_NaN();
		else
			mAnnounced.announcedExcercise.intensity = values.announced_excercise.intensity;
	}

	Emit_Signal_Level(dmms_model::signal_DMMS_IG, device_time, values.glucose_ig);
	Emit_Signal_Level(dmms_model::signal_DMMS_BG, device_time, values.glucose_bg);

	// emit announced carbs, if the time has come
	if (!std::isnan(mAnnounced.announcedMeal.time) && mAnnounced.announcedMeal.time <= device_time)
	{
		mAnnounced.announcedMeal.lastTime = mAnnounced.announcedMeal.time;
		Emit_Signal_Level(scgms::signal_Carb_Intake, mAnnounced.announcedMeal.time, mAnnounced.announcedMeal.grams);
		mAnnounced.announcedMeal.time = std::numeric_limits<double>::quiet_NaN();
	}

	// emit announced health stress, if the time has come
	if (!std::isnan(mAnnounced.announcedExcercise.time) && std::isnan(mAnnounced.announcedExcercise.lastTime) && mAnnounced.announcedExcercise.time <= device_time)
	{
		mAnnounced.announcedExcercise.lastTime = mAnnounced.announcedExcercise.time;
		Emit_Signal_Level(scgms::signal_Physical_Activity, device_time, mAnnounced.announcedExcercise.intensity);
	}
	else if (!std::isnan(mAnnounced.announcedExcercise.lastTime) && mAnnounced.announcedExcercise.lastTime + mAnnounced.announcedExcercise.duration >= device_time)
	{
		Emit_Signal_Level(scgms::signal_Physical_Activity, device_time, mAnnounced.announcedExcercise.intensity);
	}
	else
	{
		Emit_Signal_Level(scgms::signal_Physical_Activity, device_time, 0);
		mAnnounced.announcedExcercise.lastTime = std::numeric_limits<double>::quiet_NaN();
		mAnnounced.announcedExcercise.time = std::numeric_limits<double>::quiet_NaN();
	}

	Emit_Signal_Level(dmms_model::signal_DMMS_Delivered_Insulin_Basal, device_time, values.insulin_injection);

	mLastTime += time_advance_delta;

	return S_OK;
}

void CDMMS_Lib_Discrete_Model::Emit_Signal_Level(const GUID& id, double device_time, double level)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = dmms_model::proc_model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = id;
	evt.segment_id() = mSegment_Id;

	Send(evt);
}

void CDMMS_Lib_Discrete_Model::Emit_Shut_Down(double device_time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Shut_Down };

	evt.device_id() = dmms_model::proc_model_id;
	evt.device_time() = device_time;
	evt.signal_id() = Invalid_GUID;
	evt.segment_id() = mSegment_Id;

	Send(evt);
}

HRESULT IfaceCalling CDMMS_Lib_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id)
{
	mLastTime = current_time;
	mTimeStart = current_time;
	mSegment_Id = segment_id;

	return S_OK;
}

// ::ISimCallbacks iface implementation

HRESULT IfaceCalling CDMMS_Lib_Discrete_Model::iterationCallback(const dmms::SubjectObject *subjObject, const dmms::SensorSigArray *sensorSigArray, const dmms::NextMealObject *nextMealObject, const dmms::NextExerciseObject *nextExerciseObject,
	const dmms::TimeObject *timeObject, dmms::ModelInputObject *modelInputsToModObject, const dmms::InNamedSignals *inNamedSignals, dmms::OutSignalArray *outSignalArray, dmms::RunStopStatus *runStopStatus)
{
	const double bg = (*sensorSigArray)[0];
	const double cgm = (*sensorSigArray)[1];

	if (!mRunning)
		return E_FAIL;

	// lock scope
	{
		std::unique_lock<std::mutex> lck(mDmmsSimMtx);

		mToSmartCGMS.glucose_bg = bg * scgms::mgdl_2_mmoll;
		mToSmartCGMS.glucose_ig = cgm * scgms::mgdl_2_mmoll;
		mToSmartCGMS.simulation_time = timeObject->minutesPastSimStart;

		mToSmartCGMS.announced_meal.grams = nextMealObject->amountMg / 1000.0;	// mg -> g
		mToSmartCGMS.announced_meal.time = nextMealObject->minutesUntilNextMeal;

		mToSmartCGMS.announced_excercise.intensity = nextExerciseObject->intensityFrac * 1000;
		mToSmartCGMS.announced_excercise.duration = nextExerciseObject->durationInMinutes;
		mToSmartCGMS.announced_excercise.time = nextExerciseObject->minutesUntilNextSession;

		mToSmartCGMSValuesReady = true;
		mToDMMSValuesReady = false;
		mDmmsSimCv.notify_all();

		mDmmsSimCv.wait(lck, [this]() { return mToDMMSValuesReady || !mRunning; });

		if (!mRunning)
			return E_FAIL;

		modelInputsToModObject->sqInsulinNormalBasal += (mToDMMS.basal_rate / scgms::pmol_2_U) / 60.0;	// U/hr -> pmol/min
		modelInputsToModObject->sqInsulinNormalBolus += (mToDMMS.bolus_rate / scgms::pmol_2_U) / 60.0;	// U/hr -> pmol/min
		modelInputsToModObject->mealCarbsMgPerMin += mToDMMS.carbs_rate * 1000;		// g/min -> mg/min

		mToSmartCGMS.insulin_injection = mToDMMS.basal_rate / 60.0 + mToDMMS.bolus_rate / 60.0; // U/hr -> U (/1min)

		// cancel bolus
		mToDMMS.bolus_rate = 0;
	}

	return S_OK;
}

HRESULT IfaceCalling CDMMS_Lib_Discrete_Model::initializeCallback(const uint8_t useUniversalSeed, const double universalSeed, const dmms::SimInfo *simInfo)
{
	return S_OK;
}

HRESULT IfaceCalling CDMMS_Lib_Discrete_Model::outSignalName(dmms::OutputSignalNames *names)
{
	*names = static_cast<dmms::OutputSignalNames>(mDmms_Out_Signal_Names.data());

	return S_OK;
}

HRESULT IfaceCalling CDMMS_Lib_Discrete_Model::numOutputSignals(size_t *count)
{
	*count = mDmms_Out_Signal_Names.size();

	return S_OK;
}

HRESULT IfaceCalling CDMMS_Lib_Discrete_Model::cleanupCallback()
{
	return S_OK;
}
