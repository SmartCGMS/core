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

// CreateProcess, CreateFileMapping, CreateEvent, UnmapViewOfFile, CloseHandle, ResetEvent, SetEvent, WaitForSingleObject, CopyMemory
// TODO: generic implementation of IPC (this is Windows-specific)
#include <Windows.h>

#include "dmms_proc.h"
#include "../../../../common/rtl/rattime.h"
#include "../../../../common/lang/dstrings.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/FilesystemLib.h"
#include "../../../../common/utils/XMLParser.h"

//#define DSINGLE_DMMS
#ifdef DSINGLE_DMMS
	static std::mutex gGlobal_DMMS_Mtx;
	static std::condition_variable gGlobal_DMMS_Cv;
	static int gGlobal_DMMS_Instance_Cnt = 0;
#endif

CDMMS_Proc_Discrete_Model::CDMMS_Proc_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output)
	: CBase_Filter(output),
	  mParameters(scgms::Convert_Parameters<dmms_model::TParameters>(parameters, dmms_model::default_parameters.vector))
{
	mToSend.basal_rate = 0.0;
	mToSend.bolus_rate = 0.0;
	mToSend.carbs_rate = 0.0;

	Clear_DMMS_IPC(mDMMS_ipc);
	mDMMS_Proc_Info.hProcess = INVALID_HANDLE_VALUE;
}

CDMMS_Proc_Discrete_Model::~CDMMS_Proc_Discrete_Model()
{
	Deinitialize_DMMS();
}

void CDMMS_Proc_Discrete_Model::Deinitialize_DMMS() {

	Release_DMMS_IPC(mDMMS_ipc);
	
	if (mDMMS_Proc_Info.hProcess != INVALID_HANDLE_VALUE)
	{
		TerminateProcess(mDMMS_Proc_Info.hProcess, 0);
		WaitForSingleObject(mDMMS_Proc_Info.hProcess, INFINITE);
	}

	mDMMS_Proc_Info.hProcess = INVALID_HANDLE_VALUE;

	if (mDMMS_Initialized)
	{
#ifdef DSINGLE_DMMS
		std::unique_lock<std::mutex> lck(gGlobal_DMMS_Mtx);
		gGlobal_DMMS_Instance_Cnt = 0;
		gGlobal_DMMS_Cv.notify_one();
#endif

		mDMMS_Initialized = false;
	}
}

HRESULT CDMMS_Proc_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	//return S_OK;
	return Configure_DMMS() ? S_OK : E_FAIL;
}

bool CDMMS_Proc_Discrete_Model::Configure_DMMS() {
	filesystem::path root_path = Get_Application_Dir();

	auto path_to_absolute = [&root_path](const std::wstring& src_path)->std::wstring {
		filesystem::path path{ src_path };
		if (path.is_absolute()) return src_path;

		path = root_path / path;
		return filesystem::absolute(path);
	};

	
	filesystem::path mainfest_path = root_path / L"dmms_manifest.xml";
	
	CXML_Parser<wchar_t> parser(mainfest_path);
	if (!parser.Is_Valid())
		return false;

	mRun_Cmd = path_to_absolute(parser.Get_Parameter(L"manifest.dmms:exepath", L""));
	mDMMS_Scenario_File = path_to_absolute(parser.Get_Parameter(L"manifest.dmms:scenariofile", L""));
	mDMMS_Out_File = path_to_absolute(parser.Get_Parameter(L"manifest.dmms:logfile", L""));

	mRunning = mDMMS_Initialized = Initialize_DMMS();
	
	return mRunning;
}

void CDMMS_Proc_Discrete_Model::Receive_From_DMMS()
{
	TDMMS_To_SmartCGMS received;

	if (!mRunning)
		return;

	WaitForSingleObject(mDMMS_ipc.event_DataToSmartCGMS, INFINITE);

	ResetEvent(mDMMS_ipc.event_DataToSmartCGMS);

	if (!mRunning)
		return;

	CopyMemory(&received, mDMMS_ipc.filebuf_DataToSmartCGMS, sizeof(received));
	int res = sizeof(received);

	if (res == sizeof(received))
	{
		mLastSimTime = received.simulation_time;

		const double device_time = mTimeStart + scgms::One_Minute * received.simulation_time;

		if (std::isnan(mAnnounced.announcedMeal.time) && received.announced_meal.grams > 0 && received.announced_meal.time > 0)
		{
			/* meal_announce_offset - this is the time before actual meal (negative = before meal, positive = after meal)
			   we use 10 minutes before meal as default; study https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5836969/ suggests 15-20 minutes,
			   but in real scenario, it's often either exactly the time we start eating, a few minutes before that or
			   in worst case, several minutes after
			*/

			/* T1DMS performs some magic on announced meals; the original T1DMS controller checked for line-approximated
			   signal intersection with X axis and then calculated bolus; we have completely different logic, so no such
			   calculations are needed here */
			const double t1dms_compat = scgms::One_Minute * received.announced_meal.time;

			mAnnounced.announcedMeal.time = device_time + t1dms_compat + mParameters.meal_announce_offset;
			if (mAnnounced.announcedMeal.time <= mAnnounced.announcedMeal.lastTime)
				mAnnounced.announcedMeal.time = std::numeric_limits<double>::quiet_NaN();
			else
				mAnnounced.announcedMeal.grams = received.announced_meal.grams;
		}

		if (std::isnan(mAnnounced.announcedExcercise.time) && received.announced_excercise.intensity > 0 && received.announced_excercise.time > 0)
		{
			/* see note in announced meals */
			const double t1dms_compat = scgms::One_Minute * received.announced_excercise.time;

			mAnnounced.announcedExcercise.time = device_time + t1dms_compat + mParameters.excercise_announce_offset;
			mAnnounced.announcedExcercise.duration = scgms::One_Minute * received.announced_excercise.duration;
			if (mAnnounced.announcedExcercise.time <= mAnnounced.announcedExcercise.lastTime)
				mAnnounced.announcedExcercise.time = std::numeric_limits<double>::quiet_NaN();
			else
				mAnnounced.announcedExcercise.intensity = received.announced_excercise.intensity;
		}

		Emit_Signal_Level(dmms_model::signal_DMMS_IG, device_time, received.glucose_ig * scgms::mgdl_2_mmoll);
		Emit_Signal_Level(dmms_model::signal_DMMS_BG, device_time, received.glucose_bg * scgms::mgdl_2_mmoll);

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
			Emit_Signal_Level(scgms::signal_Physical_Activity, device_time, mAnnounced.announcedExcercise.intensity * 1000);
		}
		else if (!std::isnan(mAnnounced.announcedExcercise.lastTime) && mAnnounced.announcedExcercise.lastTime + mAnnounced.announcedExcercise.duration >= device_time)
		{
			Emit_Signal_Level(scgms::signal_Physical_Activity, device_time, mAnnounced.announcedExcercise.intensity * 1000);
		}
		else
		{
			Emit_Signal_Level(scgms::signal_Physical_Activity, device_time, 0);
			mAnnounced.announcedExcercise.lastTime = std::numeric_limits<double>::quiet_NaN();
			mAnnounced.announcedExcercise.time = std::numeric_limits<double>::quiet_NaN();
		}

		Emit_Signal_Level(dmms_model::signal_DMMS_Delivered_Insulin_Basal, device_time, received.insulin_injection);
	}
}

bool CDMMS_Proc_Discrete_Model::Initialize_DMMS()
{
#ifdef DSINGLE_DMMS
	std::unique_lock<std::mutex> lck(gGlobal_DMMS_Mtx);
	while (gGlobal_DMMS_Instance_Cnt != 0)
		gGlobal_DMMS_Cv.wait(lck);

	gGlobal_DMMS_Instance_Cnt = 1;
#endif

	STARTUPINFOW si;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&mDMMS_Proc_Info, sizeof(mDMMS_Proc_Info));

	std::wstring cmdLine = mRun_Cmd + L" " + mDMMS_Scenario_File + L" " + mDMMS_Out_File;

	const bool executed = CreateProcessW(NULL, (LPWSTR)cmdLine.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &mDMMS_Proc_Info) != 0;
	if (executed) {
		mDMMS_ipc = Establish_DMMS_IPC(mDMMS_Proc_Info.dwProcessId);
	}

	return executed;
}

HRESULT CDMMS_Proc_Discrete_Model::Do_Execute(scgms::UDevice_Event event)
{
	HRESULT rc = S_FALSE;

	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_Requested_Insulin_Basal_Rate)
		{
			mToSend.basal_rate = event.level();
			rc = S_OK;
		}
		else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus)
		{
			// T1DMS expects bolus rates in U/hr, and we hold the value for 5 mins => "spread" the value to 5 min dosages
			mToSend.bolus_rate += 60.0*event.level() / 5.0;
			rc = S_OK;
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		mRunning = false;

		SetEvent(mDMMS_ipc.event_DataToSmartCGMS);

		Deinitialize_DMMS();
	}

	if (rc == S_FALSE)
		rc = Send(event);

	return rc;
}

HRESULT IfaceCalling CDMMS_Proc_Discrete_Model::Step(const double time_advance_delta)
{
	if (!mRunning)
		return E_ILLEGAL_STATE_CHANGE;

	//if (!mDMMS_Initialized) Configure_DMMS();

	mToSend.device_time = mLastTime + time_advance_delta;

	// copy to shared memory
	memcpy(mDMMS_ipc.filebuf_DataFromSmartCGMS, &mToSend, sizeof(mToSend));
	// signal SmartCGMS event
	SetEvent(mDMMS_ipc.event_DataFromSmartCGMS);

	// bolus is one-time delivery - cancel it in next simulation step
	mToSend.bolus_rate = 0;

	Receive_From_DMMS();

	mLastTime += time_advance_delta;

	return S_OK;
}

void CDMMS_Proc_Discrete_Model::Emit_Signal_Level(const GUID& id, double device_time, double level)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

	evt.device_id() = dmms_model::proc_model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = id;
	evt.segment_id() = mSegment_Id;

	Send(evt);
}

void CDMMS_Proc_Discrete_Model::Emit_Shut_Down(double device_time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Shut_Down };

	evt.device_id() = dmms_model::proc_model_id;
	evt.device_time() = device_time;
	evt.signal_id() = Invalid_GUID;
	evt.segment_id() = mSegment_Id;

	Send(evt);
}

HRESULT IfaceCalling CDMMS_Proc_Discrete_Model::Initialize(const double current_time, const uint64_t segment_id)
{
	mLastTime = current_time;
	mTimeStart = current_time;
	mSegment_Id = segment_id;

	return S_OK;
}


