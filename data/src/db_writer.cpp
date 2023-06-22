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

#include "db_writer.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/ModelsLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/lang/dstrings.h"

#include "../../../common/utils/DebugHelper.h"

#include "third party/iso8601.h"
#include "descriptor.h"

#include <map>
#include <ctime>


namespace db_writer
{
	// database ID of anonymous subject; TODO: select this from database
	constexpr int64_t Anonymous_Subject_Id = 1;

	// errorneous ID
	constexpr int64_t Error_Id = -1;

	// base for segment name
	const wchar_t* Segment_Base_Name = L"Segment";

	// base for segment name
	const wchar_t* Subject_Base_Name = L"Subject";
}

CDb_Writer::CDb_Writer(scgms::IFilter* output) : CBase_Filter(output) {
	//
}

CDb_Writer::~CDb_Writer() {
	Flush_Levels();
}

HRESULT IfaceCalling CDb_Writer::QueryInterface(const GUID* riid, void** ppvObj) {

	if (Internal_Query_Interface<db::IDb_Sink>(db::Db_Sink_Filter, *riid, ppvObj))
		return S_OK;
	return E_NOINTERFACE;
}

int64_t CDb_Writer::Create_Segment(std::wstring name) {
	auto qr = mDb_Connection.Query(rsFound_New_Segment, rsReserved_Segment_Name);
	qr.Get_Next();	//no need to test the return value - shall it fail, we'll try to reuse previously founded new segment

	int64_t segment_id;
	qr = mDb_Connection.Query(rsSelect_Founded_Segment, rsReserved_Segment_Name);
	if (!qr || !qr.Get_Next(segment_id))
		return db_writer::Error_Id;

	qr = mDb_Connection.Query(rsUpdate_Founded_Segment, name.c_str(), mSubject_Id, segment_id);
	if (!qr || !qr.Get_Next())
		return db_writer::Error_Id;

	return segment_id;
}

int64_t CDb_Writer::Create_Subject(std::wstring name) {
	auto qr = mDb_Connection.Query(rsFound_New_Subject, rsReserved_Subject_Name, L"");
	qr.Get_Next();

	int64_t subject_id;
	qr = mDb_Connection.Query(rsSelect_Founded_Subject, rsReserved_Subject_Name);
	if (!qr || !qr.Get_Next(subject_id))
		return db_writer::Error_Id;

	qr = mDb_Connection.Query(rsUpdate_Founded_Subject, name.c_str(), L"", subject_id);
	if (!qr || !qr.Get_Next())
		return db_writer::Error_Id;

	return subject_id;
}

int64_t CDb_Writer::Get_Db_Segment_Id(int64_t segment_id) {

	if (mSegment_Db_Id_Map.find(segment_id) == mSegment_Db_Id_Map.end()) {
		if (mGenerate_Primary_Keys)
			mSegment_Db_Id_Map[segment_id] = Create_Segment(db_writer::Segment_Base_Name + std::wstring(L" ") + std::to_wstring(++mLast_Generated_Idx));
		else
			mSegment_Db_Id_Map[segment_id] = segment_id;
	}

	return mSegment_Db_Id_Map[segment_id];
}

bool CDb_Writer::Store_Level(const scgms::UDevice_Event& evt) {

	mPrepared_Values.push_back({
		evt.device_time(),
		evt.signal_id(),
		evt.level(),
		evt.segment_id()
		});

	return true;
}


void CDb_Writer::Flush_Levels()
{
	// TODO: transactions

	if (Succeeded(Connect_To_Db_If_Needed()))
	{
		for (const auto& val : mPrepared_Values)
		{
			const auto time_str = to_iso8601(Rat_Time_To_Unix_Time(val.measuredAt));
			auto qr = mDb_Connection.Query(rsInsert_New_Measured_Value, time_str.c_str());

			if (!qr)
				break;

			int64_t id = Get_Db_Segment_Id(val.segmentId);
			if (id == db_writer::Error_Id)
				break;

			qr.Bind_Parameters(id, val.signalId, val.value);

			qr.Execute();
		}
	}

	mPrepared_Values.clear();
}

bool CDb_Writer::Store_Parameters(const scgms::UDevice_Event& evt) {

	if (!Succeeded(Connect_To_Db_If_Needed()))
		return false;

	int64_t id = Get_Db_Segment_Id(evt.segment_id());
	if (id == db_writer::Error_Id)
		return false;

	double* begin, * end;
	bool result = (evt.parameters->get(&begin, &end) == S_OK);

	if (!result)
		return false;

	db::TBinary_Object paramblob{ static_cast<size_t>(std::distance(begin, end)) * sizeof(double), reinterpret_cast<uint8_t*>(begin) };

	const auto time_str = to_iso8601(Rat_Time_To_Unix_Time(evt.device_time()));
	auto insert_query = mDb_Connection.Query(rsInsert_Params, id);

	insert_query.Bind_Parameters(evt.device_id());
	insert_query.Bind_Parameters(evt.signal_id());
	insert_query.Bind_Parameters(time_str.c_str());
	insert_query.Bind_Parameters(paramblob);

	if (!insert_query)
		return false;

	if (!insert_query.Execute())
		return false;

	return true;
}

HRESULT IfaceCalling CDb_Writer::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	mDbHost = configuration.Read_String(rsDb_Host);
	mDbProvider = configuration.Read_String(rsDb_Provider);
	mDbPort = static_cast<decltype(mDbPort)>(configuration.Read_Int(rsDb_Port));
	mDbDatabaseName = db::is_file_db(mDbProvider) ? configuration.Read_File_Path(rsDb_Name).wstring() : configuration.Read_String(rsDb_Name);
	mDbUsername = configuration.Read_String(rsDb_User_Name);
	mDbPassword = configuration.Read_String(rsDb_Password);
	mSubject_Id = configuration.Read_Int(rsSubject_Id);

	mGenerate_Primary_Keys = configuration.Read_Bool(rsGenerate_Primary_Keys);
	mStore_Data = configuration.Read_Bool(rsStore_Data);
	mStore_Parameters = configuration.Read_Bool(rsStore_Parameters);

	// The combination of (mStore_Data && !mGenerate_Primary_Keys) may be potentially wrong (this would save the data to an existing segment),
	// should we warn the user somehow?

	// do not store any of model signals
	auto models = scgms::get_model_descriptor_list();
	for (const auto& model : models)
		for (size_t i = 0; i < model.number_of_calculated_signals; i++)
			mIgnored_Signals.insert(model.calculated_signal_ids[i]);

	// do not store virtual signals either
	for (const auto& id : scgms::signal_Virtual)
		mIgnored_Signals.insert(id);


	// bolus is an exception for now - model produces directly requested insulin bolus
	// TODO: modify model to produce model-specific signal, that needs to be remapped to requested insulin bolus signal
	mIgnored_Signals.erase(scgms::signal_Requested_Insulin_Bolus);

	switch (mSubject_Id)
	{
		case db::Anonymous_Subject_Identifier:
			// TODO: select this from DB
			mSubject_Id = db_writer::Anonymous_Subject_Id;
			break;
		case db::New_Subject_Identifier:
			mSubject_Id = Create_Subject(db_writer::Subject_Base_Name);
			break;
		default:
			// TODO: just verify, that the subject exists
			break;
	}

	return S_OK;
}


HRESULT IfaceCalling CDb_Writer::Do_Execute(scgms::UDevice_Event event) {

	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Level:
		case scgms::NDevice_Event_Code::Masked_Level:
		{
			if (mStore_Data && (mIgnored_Signals.find(event.signal_id()) == mIgnored_Signals.end())) {
				if (!Store_Level(event)) {
					dprintf(__FILE__);
					dprintf(", ");
					dprintf(__LINE__);
					dprintf(", ");
					dprintf(__func__);
					dprintf(" has failed!\n");
				}
			}
			break;
		}
		case scgms::NDevice_Event_Code::Parameters:
		{
			if (mStore_Parameters) {
				if (!Store_Parameters(event)) {
					dprintf(__FILE__);
					dprintf(", ");
					dprintf(__LINE__);
					dprintf(", ");
					dprintf(__func__);
					dprintf(" has failed!\n");
				}
			}
			break;
		}
		case scgms::NDevice_Event_Code::Time_Segment_Stop:
			Flush_Levels();
			break;
		default:
			break;
	}

	return mOutput.Send(event);
}

HRESULT IfaceCalling CDb_Writer::Set_Connector(db::IDb_Connector* connector) {
	mDb_Connector = refcnt::make_shared_reference_ext<db::SDb_Connector, db::IDb_Connector>(connector, true);

	mDb_Connection.reset();

	return connector != nullptr ? S_OK : S_FALSE;
}

HRESULT CDb_Writer::Connect_To_Db_If_Needed() {
	if (mDb_Connector)
		mDb_Connection = mDb_Connector.Connect(mDbHost, mDbProvider, mDbPort, mDbDatabaseName, mDbUsername, mDbPassword);
	if (!mDb_Connection)
		return E_FAIL;

	return S_OK;
}
