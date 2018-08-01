#include "db_writer.h"

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

#include <map>

namespace db_writer
{
	// database ID of anonymous subject; TODO: select this from database, or allow the user to select it!
	constexpr int64_t Anonymous_Subject_Id = 1;

	// errorneous ID
	constexpr int64_t Error_Id = -1;

	// base for segment name
	const wchar_t* Segment_Base_Name = L"Segment";
	// inserted time segment comment
	const wchar_t* Segment_Comment = L"";
}

CDb_Writer::CDb_Writer(glucose::SFilter_Pipe in_pipe, glucose::SFilter_Pipe out_pipe) : mInput(in_pipe), mOutput(out_pipe) {
}

HRESULT IfaceCalling CDb_Writer::QueryInterface(const GUID*  riid, void ** ppvObj) {

	if (Internal_Query_Interface<db::IDb_Sink>(db::Db_Sink_Filter, *riid, ppvObj))
		return S_OK;
	return E_NOINTERFACE;
}

int64_t CDb_Writer::Create_Segment(std::wstring name, std::wstring comment) {	
	auto qr = mDb_Connection.Query(rsFound_New_Segment, rsReserved_Segment_Name);
	qr.Get_Next();	//no need to test the return value - shall it fail, we'll try to reuse previously founded new segment

	int64_t segment_id;
	qr = mDb_Connection.Query(rsSelect_Founded_Segment, rsReserved_Segment_Name);
	if (!qr || !qr.Get_Next(segment_id))
		return db_writer::Error_Id;

	// TODO: resolve parallel segments

	qr = mDb_Connection.Query(rsUpdate_Founded_Segment, rsReserved_Segment_Name, name.c_str(), comment.c_str(), false, db_writer::Anonymous_Subject_Id, nullptr, segment_id);
	if (!qr || !qr.Get_Next(segment_id))
		return db_writer::Error_Id;

	return segment_id;
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

bool CDb_Writer::Store_Level(const glucose::UDevice_Event& evt) {

	int64_t id = Get_Db_Segment_Id(evt.segment_id);
	if (id == db_writer::Error_Id) return false;

	// "INSERT INTO measuredvalue (measuredat, blood, ist, isig, insulin, carbohydrates, calibration, segmentid) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
	auto qr = mDb_Connection.Query(rsInsert_New_Measured_Value,
		to_iso8601(Rat_Time_To_Unix_Time(evt.device_time)).c_str());

	if (!qr)
		return false;

	const auto sigCondBind = [&qr, &evt](const GUID& cond) {
		if (cond == evt.signal_id)
			qr.Bind_Parameters(evt.level);
		else
			qr.Bind_Parameters(nullptr);
	};

	sigCondBind(glucose::signal_BG);
	sigCondBind(glucose::signal_IG);
	sigCondBind(glucose::signal_ISIG);
	sigCondBind(glucose::signal_Insulin);
	sigCondBind(glucose::signal_Carb_Intake);
	sigCondBind(glucose::signal_Calibration);

	qr.Bind_Parameters(id);

	qr.Execute();

	return true;
}

bool CDb_Writer::Store_Parameters(const glucose::UDevice_Event& evt) {

	int64_t id = Get_Db_Segment_Id(evt.segment_id);
	if (id == db_writer::Error_Id) return false;

	double *begin, *end;
	evt.parameters->get(&begin, &end);

	size_t paramCnt = std::distance(begin, end);

	auto models = glucose::get_model_descriptors();
	for (const auto& model : models) {
		for (size_t i = 0; i < model.number_of_calculated_signals; i++) {
			if (evt.signal_id == model.calculated_signal_ids[i] && model.number_of_parameters == paramCnt) {

				// INSERT INTO model_db_table (segmentid, param_1, param_2, ..., param_N) VALUES (?, ?, ..., ?)

				// INSERT INTO model_db_table
				std::wstring query = rsInsert_Params_Base;
				query += std::wstring(model.db_table_name, model.db_table_name + wcslen(model.db_table_name));;

				// (segmentid, param_1, param_2, ..., param_N)
				query += L" (";
				query += rsInsert_Params_Segmentid_Column;
				for (size_t i = 0; i < model.number_of_parameters; i++)
					query += L", " + std::wstring(model.parameter_db_column_names[i], model.parameter_db_column_names[i] + wcslen(model.parameter_db_column_names[i]));
				query += L")";

				// VALUES (?, ?, ..., ?)
				query += L" ";
				query += rsInsert_Params_Values_Stmt;
				query += L" (?";
				for (size_t i = 0; i < model.number_of_parameters; i++)
					query += L", ?";
				query += L")";

				// create query and bind our parameters
				auto qr = mDb_Connection.Query(query);
				qr.Bind_Parameters(id);
				for (size_t i = 0; i < model.number_of_parameters; i++)
					qr.Bind_Parameters(*(begin + i));

				qr.Execute();
			}
		}
	}

	return true;
}

bool CDb_Writer::Configure(glucose::SFilter_Parameters conf)
{
	mDbHost = conf.Read_String(rsDb_Host);
	mDbProvider = conf.Read_String(rsDb_Provider);
	mDbPort = conf.Read_Int(rsDb_Port);
	mDbDatabaseName = conf.Read_String(rsDb_Name);
	mDbUsername = conf.Read_String(rsDb_User_Name);
	mDbPassword = conf.Read_String(rsDb_Password);

	mGenerate_Primary_Keys = conf.Read_Bool(rsGenerate_Primary_Keys);
	mStore_Data = conf.Read_Bool(rsStore_Data);
	mStore_Parameters = conf.Read_Bool(rsStore_Parameters);

	// The combination of (mStore_Data && !mGenerate_Primary_Keys) may be potentially wrong (this would save the data to an existing segment),
	// should we warn the user somehow?

	// do not store any of model signals
	auto models = glucose::get_model_descriptors();
	for (const auto& model : models)
		for (size_t i = 0; i < model.number_of_calculated_signals; i++)
			mIgnored_Signals.insert(model.calculated_signal_ids[i]);

	// do not store virtual signals either
	for (const auto& id : glucose::signal_Virtual)
		mIgnored_Signals.insert(id);

	return true;
}

HRESULT IfaceCalling CDb_Writer::Run(glucose::IFilter_Configuration *configuration)
{
	if (!Configure(refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true)))
		return E_FAIL;

	if (mDb_Connector)
		mDb_Connection = mDb_Connector.Connect(mDbHost, mDbProvider, mDbPort, mDbDatabaseName, mDbUsername, mDbPassword);
	if (!mDb_Connection)
		return E_FAIL;

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {

		switch (evt.event_code) {
			case glucose::NDevice_Event_Code::Level:
			case glucose::NDevice_Event_Code::Masked_Level:		if (mStore_Data && mIgnored_Signals.find(evt.signal_id) == mIgnored_Signals.end())
																	if (!Store_Level(evt)) return E_FAIL;
																break;

			case glucose::NDevice_Event_Code::Parameters:		if (mStore_Parameters) if (!Store_Parameters(evt)) return E_FAIL; break;
			default:	break;
		}

		if (!mOutput.Send(evt))
			break;
	}

	return S_OK;
}

HRESULT IfaceCalling CDb_Writer::Set_Connector(db::IDb_Connector *connector)
{
	mDb_Connector = refcnt::make_shared_reference_ext<db::SDb_Connector, db::IDb_Connector>(connector, true);
	return connector != nullptr ? S_OK : S_FALSE;
}
