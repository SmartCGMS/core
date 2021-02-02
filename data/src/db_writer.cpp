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
	// inserted time segment comment
	const wchar_t* Segment_Comment = L"";

	// base for segment name
	const wchar_t* Subject_Base_Name = L"Subject";
}

CDb_Writer::CDb_Writer(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CDb_Writer::~CDb_Writer() {
	Flush_Levels();
}

HRESULT IfaceCalling CDb_Writer::QueryInterface(const GUID*  riid, void ** ppvObj) {

	if (Internal_Query_Interface<db::IDb_Sink>(db::Db_Sink_Filter, *riid, ppvObj))
		return S_OK;
	return E_NOINTERFACE;
}

int64_t CDb_Writer::Create_Segment(std::wstring name, std::wstring comment) {
	auto qr = mDb_Connection.Query(rsFound_New_Segment, rsReserved_Segment_Name, L"", false);
	qr.Get_Next();	//no need to test the return value - shall it fail, we'll try to reuse previously founded new segment

	int64_t segment_id;
	qr = mDb_Connection.Query(rsSelect_Founded_Segment, rsReserved_Segment_Name);
	if (!qr || !qr.Get_Next(segment_id))
		return db_writer::Error_Id;

	qr = mDb_Connection.Query(rsUpdate_Founded_Segment, name.c_str(), comment.c_str(), false, mSubject_Id, nullptr, segment_id);
	if (!qr || !qr.Get_Next())
		return db_writer::Error_Id;

	return segment_id;
}

int64_t CDb_Writer::Create_Subject(std::wstring name) {
	auto qr = mDb_Connection.Query(rsFound_New_Subject, rsReserved_Subject_Name, L"", -1, 0);
	qr.Get_Next();

	int64_t subject_id;
	qr = mDb_Connection.Query(rsSelect_Founded_Subject, rsReserved_Subject_Name);
	if (!qr || !qr.Get_Next(subject_id))
		return db_writer::Error_Id;

	qr = mDb_Connection.Query(rsUpdate_Founded_Subject, name.c_str(), L"", 0, 0, subject_id);
	if (!qr || !qr.Get_Next())
		return db_writer::Error_Id;

	return subject_id;
}

int64_t CDb_Writer::Get_Db_Segment_Id(int64_t segment_id) {

	if (mSegment_Db_Id_Map.find(segment_id) == mSegment_Db_Id_Map.end()) {
		if (mGenerate_Primary_Keys)
			mSegment_Db_Id_Map[segment_id] = Create_Segment(db_writer::Segment_Base_Name + std::wstring(L" ") + std::to_wstring(++mLast_Generated_Idx), db_writer::Segment_Comment);
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

			const auto sigCondBind = [&qr, &val](const GUID& cond) {
				if (cond == val.signalId)
					qr.Bind_Parameters(val.value);
				else
					qr.Bind_Parameters(nullptr);
			};

			sigCondBind(scgms::signal_BG);
			sigCondBind(scgms::signal_IG);
			sigCondBind(scgms::signal_ISIG);
			sigCondBind(scgms::signal_Requested_Insulin_Bolus);
			sigCondBind(scgms::signal_Requested_Insulin_Basal_Rate);
			sigCondBind(scgms::signal_Carb_Intake);
			sigCondBind(scgms::signal_Calibration);
			sigCondBind(scgms::signal_Heartbeat);
			sigCondBind(scgms::signal_Steps);
			sigCondBind(scgms::signal_Movement_Speed);

			int64_t id = Get_Db_Segment_Id(val.segmentId);
			if (id == db_writer::Error_Id)
				break;

			qr.Bind_Parameters(id);

			qr.Execute();
		}
	}

	mPrepared_Values.clear();
}

bool CDb_Writer::Store_Parameters(const scgms::UDevice_Event& evt) {

	if (!Succeeded(Connect_To_Db_If_Needed()))
		return false;

	int64_t id = Get_Db_Segment_Id(evt.segment_id());
	if (id == db_writer::Error_Id) return false;

	double *begin, *end;	
	bool result = evt.parameters->get(&begin, &end) == S_OK;

	if (result) {
		const size_t paramCnt = std::distance(begin, end);

		auto models = scgms::get_model_descriptors();
		for (const auto& model : models) {
			for (size_t i = 0; i < model.number_of_calculated_signals; i++) {
				if (evt.signal_id() == model.calculated_signal_ids[i] && model.number_of_parameters == paramCnt) {

					// delete old parameters
					std::wstring query_text;
					query_text = rsDelete_Parameters_Of_Segment_Base;
					query_text += model.db_table_name;
					query_text += rsDelete_Parameters_Of_Segment_Stmt;

					auto delete_query = mDb_Connection.Query(query_text, id);
					if (delete_query)
						delete_query.Execute(); // TODO: error checking

					// INSERT INTO model_db_table (segmentid, param_1, param_2, ..., param_N) VALUES (?, ?, ..., ?)

					// INSERT INTO model_db_table
					query_text = rsInsert_Params_Base;
					query_text += std::wstring(model.db_table_name, model.db_table_name + wcslen(model.db_table_name));;

					// (segmentid, param_1, param_2, ..., param_N)
					query_text += L" (";
					query_text += rsInsert_Params_Segmentid_Column;
					for (size_t i = 0; i < model.number_of_parameters; i++)
						query_text += L", " + std::wstring(model.parameter_db_column_names[i], model.parameter_db_column_names[i] + wcslen(model.parameter_db_column_names[i]));
					query_text += L")";

					// VALUES (?, ?, ..., ?)
					query_text += L" ";
					query_text += rsInsert_Params_Values_Stmt;
					query_text += L" (?";
					for (size_t i = 0; i < model.number_of_parameters; i++)
						query_text += L", ?";
					query_text += L")";

					// create query and bind our parameters
					auto insert_query = mDb_Connection.Query(query_text);
					if (insert_query) {
						insert_query.Bind_Parameters(id);
						for (size_t i = 0; i < model.number_of_parameters; i++)
							insert_query.Bind_Parameters(*(begin + i));

						result = insert_query.Execute();
					}
				}
			}
		}
	}

	return result;
}

HRESULT IfaceCalling CDb_Writer::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	mDbHost = configuration.Read_String(rsDb_Host);
	mDbProvider = configuration.Read_String(rsDb_Provider);
	mDbPort = static_cast<decltype(mDbPort)>(configuration.Read_Int(rsDb_Port));
	mDbDatabaseName = configuration.Read_String(rsDb_Name);
	mDbUsername = configuration.Read_String(rsDb_User_Name);
	mDbPassword = configuration.Read_String(rsDb_Password);
	mSubject_Id = configuration.Read_Int(rsSubject_Id);

	mGenerate_Primary_Keys = configuration.Read_Bool(rsGenerate_Primary_Keys);
	mStore_Data = configuration.Read_Bool(rsStore_Data);
	mStore_Parameters = configuration.Read_Bool(rsStore_Parameters);

	// The combination of (mStore_Data && !mGenerate_Primary_Keys) may be potentially wrong (this would save the data to an existing segment),
	// should we warn the user somehow?

	// do not store any of model signals
	auto models = scgms::get_model_descriptors();
	for (const auto& model : models)
		for (size_t i = 0; i < model.number_of_calculated_signals; i++)
			mIgnored_Signals.insert(model.calculated_signal_ids[i]);

	// do not store virtual signals either
	for (const auto& id : scgms::signal_Virtual)
		mIgnored_Signals.insert(id);

	// bolus is an exception for now - model produces directly requested insulin bolus
	// TODO: modify model to produce model-specific signal, that needs to be remapped to requested insulin bolus signal
	mIgnored_Signals.erase(scgms::signal_Requested_Insulin_Bolus);

	return S_OK;
}


HRESULT IfaceCalling CDb_Writer::Do_Execute(scgms::UDevice_Event event) {

	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Level:
		case scgms::NDevice_Event_Code::Masked_Level:
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

		case scgms::NDevice_Event_Code::Parameters:		if (mStore_Parameters) {
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

		case scgms::NDevice_Event_Code::Time_Segment_Stop:	Flush_Levels();
																break;
		default:	break;
	}

	return Send(event);
}

HRESULT IfaceCalling CDb_Writer::Set_Connector(db::IDb_Connector *connector)
{
	if (!connector)
		return E_INVALIDARG;

	mDb_Connector = refcnt::make_shared_reference_ext<db::SDb_Connector, db::IDb_Connector>(connector, true);

	return S_OK;
}

HRESULT CDb_Writer::Connect_To_Db_If_Needed()
{
	if (mConnected)
		return S_FALSE;

	if (mDb_Connector)
		mDb_Connection = mDb_Connector.Connect(mDbHost, mDbProvider, mDbPort, mDbDatabaseName, mDbUsername, mDbPassword);

	if (!mDb_Connection)
		return E_FAIL;

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
