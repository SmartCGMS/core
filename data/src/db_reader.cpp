#include "db_reader.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/ModelsLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/lang/dstrings.h"

#include "fileloader/Extractor.h"

#include <tbb/tbb_allocator.h>

#include <map>

#include <QtCore/QDate>
#include <QtCore/QDateTime>


// dummy device GUID
const GUID Db_Reader_Device_GUID = { 0x00000001, 0x0001, 0x0001,{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

// enumerator of known column indexes
enum class NColumn_Pos : int
{
	Blood = 0,
	Ist,
	Isig,
	Insulin,
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

// map of DB columns to signal GUIDs used
std::map<NColumn_Pos, GUID, std::less<NColumn_Pos>, tbb::tbb_allocator<std::pair<NColumn_Pos, GUID>>> ColumnSignalMap = {
	{ NColumn_Pos::Blood, glucose::signal_BG },
	{ NColumn_Pos::Ist, glucose::signal_IG },
	{ NColumn_Pos::Isig, glucose::signal_ISIG },
	{ NColumn_Pos::Insulin, glucose::signal_Insulin },
	{ NColumn_Pos::Carbohydrates, glucose::signal_Carb_Intake },
	{ NColumn_Pos::Calibration, glucose::signal_Calibration }
};

CDb_Reader::CDb_Reader(glucose::SFilter_Pipe in_pipe, glucose::SFilter_Pipe out_pipe) : mInput(in_pipe), mOutput(out_pipe), mDbPort(0), mCurrentSegmentIdx(glucose::Invalid_Segment_Id) {
	//
}

CDb_Reader::~CDb_Reader()
{
	//
}

void CDb_Reader::Send_Segment_Marker(glucose::NDevice_Event_Code code, double device_time, uint64_t segment_id)
{
	glucose::UDevice_Event evt{ code };

	evt.device_id = Db_Reader_Device_GUID;
	evt.device_time = device_time;	
	evt.segment_id = segment_id;

	mOutput.Send(evt);
}

void CDb_Reader::Run_Reader() {

	//by consulting
	//https://stackoverflow.com/questions/47457478/using-qsqlquery-from-multiple-threads?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
	//https://doc.qt.io/qt-5/threads-modules.html

	//we must open the db connection from exactly that thread, that is about to use it

	HRESULT rc = E_FAIL;
	if (mDb_Connector)
		mDb_Connection = mDb_Connector.Connect(mDbHost, mDbProvider, mDbPort, mDbDatabaseName, mDbUsername, mDbPassword);
	if (!mDb_Connection)
		return;

	time_t unixts;

	int64_t currentSegmentId;
	
	double nowdate = Unix_Time_To_Rat_Time(QDateTime::currentDateTime().toSecsSinceEpoch());
	double begindate = nowdate;

	NKnownDateFormat dateFmt = NKnownDateFormat::UNKNOWN_DATEFORMAT;

	// initial setting query
	{
		std::wstring qstr = rsSelect_Timesegment_Values_Filter;

		db::SDb_Query query = mDb_Connection.Query(qstr, mDbTimeSegmentIds[0]);
		wchar_t *dt_str = nullptr;

		if (query.Get_Next(dt_str)) {
			
			dateFmt = Recognize_Date_Format(dt_str);

			// this needs to be resolved!
			if (dateFmt == NKnownDateFormat::UNKNOWN_DATEFORMAT)
				return;

			std::string tmp;
			if (!Str_Time_To_Unix_Time(dt_str, dateFmt, tmp, nullptr, unixts))
				return;

			begindate = Unix_Time_To_Rat_Time(unixts);
		}
		else
			return;
	}

	// base correction is used as value for shifting the time segment (its values) from past to present
	// adding this to every value will cause the segment to begin with the first value as if it was measured just now
	// and every next value will be shifted by exactly the same amount of time, so the spacing is preserved
	// this allows artificial slowdown filter to simulate real-time measurement without need of real device
	double dateCorrection = nowdate - begindate;

	double lastDate = begindate + dateCorrection;

	bool isAborted = false;


	for (size_t idx = 0; idx < mDbTimeSegmentIds.size(); idx++) {
		currentSegmentId = (int64_t)mDbTimeSegmentIds[idx];

		wchar_t* target;
		std::vector<double> bBound(static_cast<size_t>(NColumn_Pos::_Count));

		db::SDb_Query query = mDb_Connection.Query(rsSelect_Timesegment_Values_Filter, currentSegmentId);		
		if (!query.Bind_Result(target, bBound)) continue;
		
		if (!query.Get_Next())
			continue;


		std::string bDate;

		if (!Str_Time_To_Unix_Time(target, dateFmt, bDate, nullptr, unixts))
			continue;

		begindate = Unix_Time_To_Rat_Time(unixts) + dateCorrection;

		// if the spacing is greated than 1 day, condense it
		if (abs(begindate - lastDate) > 1.0)
		{
			// but preserve daily alignment (that what happened at 14:32 will happen at 14:32, but next day)
			dateCorrection -= ceil(begindate - lastDate);
			dateCorrection += 1.0;

			begindate = Unix_Time_To_Rat_Time(unixts) + dateCorrection;
		}

		Send_Segment_Marker(glucose::NDevice_Event_Code::Time_Segment_Start, begindate, currentSegmentId);		

		std::vector<TStored_Model_Params> paramHints;
		Prepare_Model_Parameters_For(currentSegmentId, paramHints);

		// at the beginning of the segment, send model parameters hint to chain
		// this serves as initial estimation for models in chain
		for (auto& paramset : paramHints)
		{
			glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Parameters_Hint };
			evt.device_time = begindate;
			evt.signal_id = paramset.model_id;
			evt.parameters = paramset.params;
			evt.segment_id = currentSegmentId;

			if (!mOutput.Send(evt))
			{
				isAborted = true;
				break;
			}
		}

		do
		{
			// rsSelect_Timesegment_Values_Filter
			// "select measuredat, blood, ist, isig, insulin, carbohydrates, calibration from measuredvalue where segmentid = ? order by measuredat asc"
			//         0           1      2    3     4        5              6

			
			if (!Str_Time_To_Unix_Time(target , dateFmt, bDate, nullptr, unixts))
				continue;

			double jdate = Unix_Time_To_Rat_Time(unixts) + dateCorrection;

			// go through all value columns
			for (NColumn_Pos i = NColumn_Pos::_Begin; i < NColumn_Pos::_End; ++i)
			{
				auto column = bBound[static_cast<size_t>(i)];

				// if no value is present, skip
				auto fpcl = fpclassify(column);
				if (fpcl == FP_NAN || fpcl == FP_INFINITE)
					continue;

				glucose::UDevice_Event evt{ (i == NColumn_Pos::Calibration) ? glucose::NDevice_Event_Code::Calibrated : glucose::NDevice_Event_Code::Level };

				evt.level = column;
				evt.device_id = Db_Reader_Device_GUID;
				evt.signal_id = ColumnSignalMap[i];
				evt.device_time = lastDate = jdate + dateCorrection; // apply base correction
				evt.segment_id = currentSegmentId;
				 
				// this may block if the pipe is full (i.e. due to artificial slowdown filter, simulation stepping, etc.)
				if (!mOutput.Send(evt))
				{
					isAborted = true;
					break;
				}
			}
		} while (!isAborted && query.Get_Next());

		// evt.device_time is now guaranteed to have valid time of last sent event

		Send_Segment_Marker(glucose::NDevice_Event_Code::Time_Segment_Stop, lastDate, currentSegmentId);
	}
}

void CDb_Reader::Run_Main() {
	
	for (; glucose::UDevice_Event evt = mInput.Receive(); evt) {
		// just fall through in main filter thread
		// there also may be some control code handling (i.e. pausing value sending, etc.)

		if (!mOutput.Send(evt))
			break;
	}

	if (mReaderThread->joinable())
		mReaderThread->join();
}

HRESULT CDb_Reader::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) {

	wchar_t *begin, *end;

	glucose::TFilter_Parameter *cbegin, *cend;
	if (configuration->get(&cbegin, &cend) != S_OK)
		return E_FAIL;

	// lambda for common code (convert wstr_container to wstring)
	auto wstrConv = [&begin, &end](glucose::TFilter_Parameter const* param) -> std::wstring {

		if (param->wstr->get(&begin, &end) != S_OK)
			return std::wstring();

		return std::wstring(begin, end);
	};

	for (glucose::TFilter_Parameter* cur = cbegin; cur < cend; cur += 1)
	{
		wchar_t *begin, *end;
		if (cur->config_name->get(&begin, &end) != S_OK)
			continue;

		std::wstring confname{ begin, end };

		if (confname == rsDb_Host)
			mDbHost = wstrConv(cur);
		else if (confname == rsDb_Provider)
			mDbProvider = wstrConv(cur);
		else if (confname == rsDb_Port)
			mDbPort = static_cast<uint16_t>(cur->int64);
		else if (confname == rsDb_Name)
			mDbDatabaseName = wstrConv(cur);
		else if (confname == rsDb_User_Name)
			mDbUsername = wstrConv(cur);
		else if (confname == rsDb_Password)
			mDbPassword = wstrConv(cur);
		else if (confname == rsTime_Segment_ID)
		{
			auto ref = cur->select_time_segment_id;

			int64_t *b, *e;
			ref->get(&b, &e);

			while (b != e)
				mDbTimeSegmentIds.push_back(*b++);
		}
	}


	// we need at least these parameters
	if (mDbHost.empty() || mDbProvider.empty() || mDbTimeSegmentIds.empty())
		return E_INVALIDARG;

	//as we have to open the db from the reader thread, we have no other way than to assume that everything goes fine
	mReaderThread = std::make_unique<std::thread>(&CDb_Reader::Run_Reader, this);
	Run_Main();

	return S_OK;
}

void CDb_Reader::Prepare_Model_Parameters_For(int64_t segmentId, std::vector<TStored_Model_Params> &paramsTarget) {

	for (const auto &descriptor : glucose::get_model_descriptors()) {
		std::wstring qry = rsSelect_Params_Base;

		bool first = true;
		for (size_t i = 0; i < descriptor.number_of_parameters; i++)
		{
			if (first)
				first = false;
			else
				qry += L", ";

			qry += std::wstring(descriptor.parameter_db_column_names[i], descriptor.parameter_db_column_names[i] + wcslen(descriptor.parameter_db_column_names[i]));
		}

		qry += L" ";
		qry += rsSelect_Params_From;
		qry += std::wstring(descriptor.db_table_name, descriptor.db_table_name + wcslen(descriptor.db_table_name));
		qry += rsSelect_Params_Condition;
		db::SDb_Query squery = mDb_Connection.Query(qry, segmentId);

		std::vector<double> sql_result (descriptor.number_of_parameters);
		if (squery.Bind_Result(sql_result)) {
			if (squery.Get_Next()) {
				paramsTarget.push_back({ descriptor.id, refcnt::Create_Container_shared<double>(sql_result.data(), sql_result.data() + sql_result.size()) });
			}
		}
	}
}


HRESULT IfaceCalling CDb_Reader::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<db::IDb_Sink>(db::Db_Sink_Filter, *riid, ppvObj)) return S_OK;
	return E_NOINTERFACE;
}

HRESULT IfaceCalling CDb_Reader::Set_Connector(db::IDb_Connector *connector) {
	mDb_Connector = refcnt::make_shared_reference_ext<db::SDb_Connector, db::IDb_Connector>(connector, true);
	return connector != nullptr ? S_OK : S_FALSE;
}