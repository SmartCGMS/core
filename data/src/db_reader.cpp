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

#include <tbb/tbb_allocator.h>

#include <cmath>
#include <map>

const GUID Db_Reader_Device_GUID = db_reader::filter_id;// { 0x00000001, 0x0001, 0x0001, { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

// enumerator of known column indexes
enum class NColumn_Pos : size_t
{
	Blood = 0,
	Ist,
	Isig,
	Insulin_Bolus,
	Insulin_Basal_Rate,
	Carbohydrates,
	Calibration,
	_End,

	_Begin = Blood,
	_Count = _End
};

// increment operator to allow using for loop
NColumn_Pos& operator++(NColumn_Pos& ref)
{
	ref = static_cast<NColumn_Pos>(static_cast<int>(ref) + 1);
	return ref;
}

// array of DB columns - signal GUIDs used
std::array<GUID, static_cast<size_t>(NColumn_Pos::_Count)> ColumnSignalMap = { {
	glucose::signal_BG, glucose::signal_IG, glucose::signal_ISIG, glucose::signal_Bolus_Insulin, glucose::signal_Basal_Insulin_Rate, glucose::signal_Carb_Intake, glucose::signal_Calibration
} };

CDb_Reader::CDb_Reader(glucose::SFilter_Pipe in_pipe, glucose::SFilter_Pipe out_pipe) : mInput(in_pipe), mOutput(out_pipe), mDbPort(0) {
	//
}

bool CDb_Reader::Emit_Shut_Down()
{
	glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Shut_Down };

	evt.device_id() = Db_Reader_Device_GUID;

	return mOutput.Send(evt);
}

bool CDb_Reader::Emit_Segment_Marker(glucose::NDevice_Event_Code code, int64_t segment_id) {
	glucose::UDevice_Event evt{ code };

	evt.device_id() = Db_Reader_Device_GUID;
	evt.segment_id() = segment_id;

	return mOutput.Send(evt);
}

bool CDb_Reader::Emit_Segment_Parameters(int64_t segment_id) {
	for (const auto &descriptor : glucose::get_model_descriptors()) {

		// skip models without database schema tables
		if (descriptor.db_table_name == nullptr || wcslen(descriptor.db_table_name) == 0)
			continue;

		std::wstring query = rsSelect_Params_Base;

		for (size_t i = 0; i < descriptor.number_of_parameters; i++) {

			if (i>0) query += L", ";
			query += std::wstring(descriptor.parameter_db_column_names[i], descriptor.parameter_db_column_names[i] + wcslen(descriptor.parameter_db_column_names[i]));
		}

		query += L" ";
		query += rsSelect_Params_From;
		query += std::wstring(descriptor.db_table_name, descriptor.db_table_name + wcslen(descriptor.db_table_name));
		query += rsSelect_Params_Condition;

		try
		{
			db::SDb_Query squery = mDb_Connection.Query(query, segment_id);

			std::vector<double> sql_result(descriptor.number_of_parameters);
			if (squery.Bind_Result(sql_result)) {
				if (squery.Get_Next()) {
					// TODO: disambiguate stored parameters for different signals within one model! This i.e. mixes diffusion2 blood and ist parameters
					for (size_t i = 0; i < descriptor.number_of_calculated_signals; i++)
					{
						glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Parameters };
						evt.device_id() = Db_Reader_Device_GUID;
						evt.signal_id() = descriptor.calculated_signal_ids[i];
						evt.segment_id() = segment_id;
						if (evt.parameters.set(sql_result))
							if (!mOutput.Send(evt)) return false;
					}
				}
			}
		}
		catch (...)
		{
			/* this probably means the model does not have the table created yet */
		}
	}

	return true;
}



bool CDb_Reader::Emit_Segment_Levels(int64_t segment_id) {
	wchar_t* measured_at_str;


	std::vector<double> levels(static_cast<size_t>(NColumn_Pos::_Count));

	db::SDb_Query query = mDb_Connection.Query(rsSelect_Timesegment_Values_Filter, segment_id);
	if (!query.Bind_Result(measured_at_str, levels)) return false;

	while (query.Get_Next() && !mQuit_Flag) {

		// "select measuredat, blood, ist, isig, insulin_bolus, insulin_basal_rate, carbohydrates, calibration from measuredvalue where segmentid = ? order by measuredat asc"
		//         0           1      2    3     4              5                   6              7

		// assuming that subsequent datetime formats are identical, try to recognize the used date time format from the first line
		const double measured_at = Unix_Time_To_Rat_Time(from_iso8601(measured_at_str));
		if (measured_at == 0.0) continue;	// conversion did not succeed

		// go through all value columns
		for (NColumn_Pos i = NColumn_Pos::_Begin; i < NColumn_Pos::_End; ++i) {
			const auto column = levels[static_cast<size_t>(i)];

			// if no value is present, skip
			const auto fpcl = std::fpclassify(column);
			if (fpcl == FP_NAN || fpcl == FP_INFINITE)
				continue;

			glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Level };

			evt.level() = column;
			evt.device_id() = Db_Reader_Device_GUID;
			evt.signal_id() = ColumnSignalMap[static_cast<size_t>(i)];
			evt.device_time() = measured_at;
			evt.segment_id() = segment_id;

			// this may block if the pipe is full (i.e. due to artificial slowdown filter, simulation stepping, etc.)
			if (!mOutput.Send(evt)) return false;
		}
	}
	return true;
}

void CDb_Reader::Db_Reader() {
	mQuit_Flag = false;

	//by consulting
	//https://stackoverflow.com/questions/47457478/using-qsqlquery-from-multiple-threads?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
	//https://doc.qt.io/qt-5/threads-modules.html

	//we must open the db connection from exactly that thread, that is about to use it

	if (mDb_Connector)
		mDb_Connection = mDb_Connector.Connect(mDbHost, mDbProvider, mDbPort, mDbDatabaseName, mDbUsername, mDbPassword);
	if (!mDb_Connection)
		return;



	for (const auto segment_index : mDbTimeSegmentIds) {
		if (!Emit_Segment_Marker(glucose::NDevice_Event_Code::Time_Segment_Start, segment_index)) break;
		if (!Emit_Segment_Parameters(segment_index)) break;
		if (!Emit_Segment_Levels(segment_index)) break;
		if (!Emit_Segment_Marker(glucose::NDevice_Event_Code::Time_Segment_Stop, segment_index)) break;
	}

	if (mShutdownAfterLast)
		Emit_Shut_Down();
}

bool CDb_Reader::Configure(glucose::SFilter_Parameters configuration) {
	mDbHost = configuration.Read_String(rsDb_Host);
	mDbProvider = configuration.Read_String(rsDb_Provider);
	mDbPort = configuration.Read_Int(rsDb_Port);
	mDbDatabaseName = configuration.Read_String(rsDb_Name);
	mDbUsername = configuration.Read_String(rsDb_User_Name);
	mDbPassword = configuration.Read_String(rsDb_Password);
	mDbTimeSegmentIds = configuration.Read_Int_Array(rsTime_Segment_ID);
	mShutdownAfterLast = configuration.Read_Bool(rsShutdown_After_Last);

	// we need at least these parameters
	return !(mDbHost.empty() || mDbProvider.empty() || mDbTimeSegmentIds.empty());
}

HRESULT CDb_Reader::Run(glucose::IFilter_Configuration *configuration) {

	if (!Configure(refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true))) return E_INVALIDARG;

	//run the db-reader thread and meanwhile jsut forward the messages as they come
	mDb_Reader_Thread = std::make_unique<std::thread>(&CDb_Reader::Db_Reader, this);


	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
		if (evt) {
			if (evt.event_code() == glucose::NDevice_Event_Code::Warm_Reset) {
				//recreate the reader thread
				End_Db_Reader();
				mDb_Reader_Thread = std::make_unique<std::thread>(&CDb_Reader::Db_Reader, this);
			}


			if (!mOutput.Send(evt)) 
				break;	//passing the shutdown code will shutdown the outpipe and subsequently the db-reader thread as well
		}
	}


	End_Db_Reader();

	return S_OK;
}

void CDb_Reader::End_Db_Reader() {
	mQuit_Flag = true;
	if (mDb_Reader_Thread->joinable())
		mDb_Reader_Thread->join();
}

HRESULT IfaceCalling CDb_Reader::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<db::IDb_Sink>(db::Db_Sink_Filter, *riid, ppvObj)) return S_OK;
	return E_NOINTERFACE;
}

HRESULT IfaceCalling CDb_Reader::Set_Connector(db::IDb_Connector *connector) {
	mDb_Connector = refcnt::make_shared_reference_ext<db::SDb_Connector, db::IDb_Connector>(connector, true);
	return connector != nullptr ? S_OK : S_FALSE;
}
