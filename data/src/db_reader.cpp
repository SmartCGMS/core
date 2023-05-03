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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "db_reader.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/ModelsLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/lang/dstrings.h"

#include "third party/iso8601.h"
#include "descriptor.h"

#include <cmath>
#include <map>

const GUID Db_Reader_Device_GUID = db_reader::filter_id;

CDb_Reader::CDb_Reader(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CDb_Reader::~CDb_Reader() {
	End_Db_Reader();
}

bool CDb_Reader::Emit_Shut_Down() {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Shut_Down };
	evt.device_id() = Db_Reader_Device_GUID;
	return Succeeded(mOutput.Send(evt));
}

bool CDb_Reader::Emit_Segment_Marker(const scgms::NDevice_Event_Code code, const double device_time, const int64_t segment_id) {
	scgms::UDevice_Event evt{ code };

	evt.device_time() = device_time;
	evt.device_id() = Db_Reader_Device_GUID;
	evt.segment_id() = segment_id;

	return Succeeded(mOutput.Send(evt));
}

bool CDb_Reader::Emit_Segment_Parameters(const double deviceTime, int64_t segment_id) {

	// "select recorded_at, model_id, signal_id, parameters from model_parameters where time_segment_id = ?"

	wchar_t* recorded_at_str;
	GUID signal_id, model_id;
	db::TBinary_Object blob;

	db::SDb_Query query = mDb_Connection.Query(rsSelect_Params_Filter, segment_id);
	if (!query.Bind_Result(recorded_at_str, model_id, signal_id, blob))
		return false;

	while (query.Get_Next() && !mQuit_Flag) {

		if (!recorded_at_str)
			continue;

		const double recorded_at = Unix_Time_To_Rat_Time(from_iso8601(recorded_at_str));
		if (recorded_at == 0.0)
			continue;	// conversion did not succeed

		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Parameters };

		double* vals = reinterpret_cast<double*>(blob.data);
		const size_t valcount = blob.size / sizeof(double);
		if (valcount == 0)
			continue;

		std::vector<double> datavec{ vals, vals + valcount };

		evt.parameters.set(datavec);
		evt.device_id() = model_id;
		evt.signal_id() = signal_id;
		evt.device_time() = recorded_at;
		evt.segment_id() = segment_id;

		if (mOutput.Send(evt) != S_OK)
			return false;
	}

	return true;
}

bool CDb_Reader::Emit_Segment_Levels(int64_t segment_id) {
	wchar_t* measured_at_str;
	GUID signal_id;
	double level;

	db::SDb_Query query = mDb_Connection.Query(rsSelect_Timesegment_Values_Filter, segment_id);
	if (!query.Bind_Result(measured_at_str, signal_id, level)) return false;

	double lastTime = std::numeric_limits<double>::quiet_NaN();

	while (query.Get_Next() && !mQuit_Flag) {

		// it is somehow possible for date being null (although the database constraint doesn't allow it)
		// TODO: revisit this after database restructuralisation
		if (!measured_at_str)
			continue;

		// "select measured_at, signal_id, value from measured_value where time_segment_id = ? order by measured_at asc"
		//         0            1          2

		const double measured_at = Unix_Time_To_Rat_Time(from_iso8601(measured_at_str));
		auto fpcl = std::fpclassify(measured_at);
		if (fpcl == FP_ZERO || fpcl == FP_NAN)
			continue;	// conversion did not succeed or yielded an invalid result

		// validate the level
		fpcl = std::fpclassify(level);
		if (fpcl == FP_NAN || fpcl == FP_INFINITE)
			continue;

		if (std::isnan(lastTime)) {
			if (!Emit_Segment_Marker(scgms::NDevice_Event_Code::Time_Segment_Start, measured_at, segment_id)) {
				return false;
			}

			if (!Emit_Segment_Parameters(measured_at, segment_id)) {
				break;
			}
		}

		lastTime = measured_at;

		{
			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };

			evt.level() = level;
			evt.device_id() = Db_Reader_Device_GUID;
			evt.signal_id() = signal_id;
			evt.device_time() = measured_at;
			evt.segment_id() = segment_id;

			if (mOutput.Send(evt) != S_OK)
				return false;
		}
	}

	return Emit_Segment_Marker(scgms::NDevice_Event_Code::Time_Segment_Stop, lastTime, segment_id);
}

bool CDb_Reader::Emit_Info_Event(const std::wstring& info)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Information };

	evt.device_id() = Db_Reader_Device_GUID;
	evt.signal_id() = Invalid_GUID;
	evt.device_time() = Unix_Time_To_Rat_Time(time(nullptr));
	evt.info.set(info.c_str());

	return (Succeeded(mOutput.Send(evt)));
}

void CDb_Reader::Db_Reader() {
	
	mQuit_Flag = false;

	// by consulting
	// https://stackoverflow.com/questions/47457478/using-qsqlquery-from-multiple-threads?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
	// https://doc.qt.io/qt-5/threads-modules.html

	// we must open the db connection from exactly that thread, that is about to use it

	if (mDb_Connector)
		mDb_Connection = mDb_Connector.Connect(mDbHost, mDbProvider, mDbPort, mDbDatabaseName, mDbUsername, mDbPassword);
	if (!mDb_Connection)
	{
		Emit_Info_Event(dsError_Could_Not_Connect_To_Db);
		return;
	}

	for (const auto segment_index : mDbTimeSegmentIds)
	{
		Emit_Segment_Levels(segment_index);
	}

	if (mShutdownAfterLast)
		Emit_Shut_Down();
}

HRESULT IfaceCalling CDb_Reader::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDbHost = configuration.Read_String(rsDb_Host);
	mDbProvider = configuration.Read_String(rsDb_Provider);
	mDbPort = static_cast<decltype(mDbPort)>(configuration.Read_Int(rsDb_Port));
	mDbDatabaseName = db::is_file_db(mDbProvider) ? configuration.Read_File_Path(rsDb_Name).wstring() : configuration.Read_String(rsDb_Name);
	mDbUsername = configuration.Read_String(rsDb_User_Name);
	mDbPassword = configuration.Read_String(rsDb_Password);
	mDbTimeSegmentIds = configuration.Read_Int_Array(rsTime_Segment_ID);
	mShutdownAfterLast = configuration.Read_Bool(rsShutdown_After_Last);

	if (mDbTimeSegmentIds.empty()) {
		error_description.push(dsNo_Time_Segments_Specified);
		return E_INVALIDARG;
	}

	return S_OK;
}

HRESULT IfaceCalling CDb_Reader::Do_Execute(scgms::UDevice_Event event) {

	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Warm_Reset: 
			// recreate the reader thread
			End_Db_Reader();
			mDb_Reader_Thread = std::make_unique<std::thread>(&CDb_Reader::Db_Reader, this);
			break;

		case scgms::NDevice_Event_Code::Shut_Down:
			mQuit_Flag = true;
			break;

		default:
			break;
	}

	return mOutput.Send(event);
}

void CDb_Reader::End_Db_Reader() {
	mQuit_Flag = true;
	if (mDb_Reader_Thread)
	{
		if (mDb_Reader_Thread->joinable())
			mDb_Reader_Thread->join();
	}
}

HRESULT IfaceCalling CDb_Reader::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<db::IDb_Sink>(db::Db_Sink_Filter, *riid, ppvObj)) return S_OK;
	return E_NOINTERFACE;
}

HRESULT IfaceCalling CDb_Reader::Set_Connector(db::IDb_Connector *connector) {
	if (!connector)
		return E_INVALIDARG;
	mDb_Connector = refcnt::make_shared_reference_ext<db::SDb_Connector, db::IDb_Connector>(connector, true);
	
	// we need at least these parameters
	HRESULT rc = (!(mDbHost.empty() || mDbProvider.empty() || mDbTimeSegmentIds.empty())) ? S_OK : E_INVALIDARG;
	if (rc == S_OK)
		mDb_Reader_Thread = std::make_unique<std::thread>(&CDb_Reader::Db_Reader, this);

	return rc;
}
