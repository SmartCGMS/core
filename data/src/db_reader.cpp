#include "db_reader.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/ModelsLib.h"
#include "../../../common/rtl/UILib.h"

#include "../../../common/lang/dstrings.h"

#include <map>

#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtSql/QSqlError>
#include <QtCore/QCoreApplication>

// dummy device GUID
const GUID Db_Reader_Device_GUID = { 0x00000001, 0x0001, 0x0001,{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

// enumerator of known column indexes
enum class NColumn_Pos : int
{
	Blood = 1,
	Ist,
	Isig,
	Insulin,
	Carbohydrates,
	Calibration,
	_End,

	_Begin = Blood
};

// increment operator to allow using for loop
NColumn_Pos& operator++(NColumn_Pos& ref)
{
	ref = static_cast<NColumn_Pos>(static_cast<int>(ref) + 1);
	return ref;
}

// map of DB columns to signal GUIDs used
std::map<NColumn_Pos, GUID> ColumnSignalMap = {
	{ NColumn_Pos::Blood, glucose::signal_BG },
	{ NColumn_Pos::Ist, glucose::signal_IG },
	{ NColumn_Pos::Isig, glucose::signal_ISIG },
	{ NColumn_Pos::Insulin, glucose::signal_Insulin },
	{ NColumn_Pos::Carbohydrates, glucose::signal_Carb_Intake },
	{ NColumn_Pos::Calibration, glucose::signal_Calibration }
};

CDb_Reader::CDb_Reader(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
	: mInput(inpipe), mOutput(outpipe), mDbPort(0), mCurrentSegmentIdx(-1)
{
	//
	QCoreApplication::addLibraryPath("c:\\programy\\glucose3\\compiled\\sqldrivers");
}

CDb_Reader::~CDb_Reader()
{
	//
}

void CDb_Reader::Send_Segment_Marker(glucose::NDevice_Event_Code code, double device_time, int64_t logical_time, uint64_t segment_id)
{
	glucose::TDevice_Event evt;

	evt.device_id = Db_Reader_Device_GUID;
	evt.device_time = device_time;
	evt.event_code = code;
	//evt.logical_time = logical_time;
	evt.segment_id = segment_id;

	mOutput->send(&evt);
}

void CDb_Reader::Run_Reader()
{
	bool ok;
	glucose::TDevice_Event evt;
	int64_t logicalTime = 1;
	uint64_t currentSegmentId;

	mValueQuery[0]->seek(0);

	double begindate = Unix_Time_To_Rat_Time(mValueQuery[0]->value(0).toDateTime().toSecsSinceEpoch());
	double nowdate = Unix_Time_To_Rat_Time(QDateTime::currentDateTime().toSecsSinceEpoch());

	// base correction is used as value for shifting the time segment (its values) from past to present
	// adding this to every value will cause the segment to begin with the first value as if it was measured just now
	// and every next value will be shifted by exactly the same amount of time, so the spacing is preserved
	// this allows artificial slowdown filter to simulate real-time measurement without need of real device
	double dateCorrection = nowdate - begindate;

	double lastDate = begindate + dateCorrection;

	for (size_t idx = 0; idx < mDbTimeSegmentIds.size(); idx++)
	{
		currentSegmentId = (uint64_t)mDbTimeSegmentIds[idx];

		// seek to first record in result set
		mValueQuery[idx]->seek(0);

		begindate = Unix_Time_To_Rat_Time(mValueQuery[idx]->value(0).toDateTime().toSecsSinceEpoch()) + dateCorrection;

		// if the spacing is greated than 1 day, condense it
		if (abs(begindate - lastDate) > 1.0)
		{
			// but preserve daily alignment (that what happened at 14:32 will happen at 14:32, but next day)
			dateCorrection -= ceil(begindate - lastDate);
			dateCorrection += 1.0;

			begindate = Unix_Time_To_Rat_Time(mValueQuery[idx]->value(0).toDateTime().toSecsSinceEpoch()) + dateCorrection;
		}

		Send_Segment_Marker(glucose::NDevice_Event_Code::Time_Segment_Start, begindate, logicalTime, currentSegmentId);
		logicalTime++;

		evt.device_time = begindate;

		std::vector<StoredModelParams> paramHints;
		Prepare_Model_Parameters_For(currentSegmentId, paramHints);

		// at the beginning of the segment, send model parameters hint to chain
		// this serves as initial estimation for models in chain
		for (auto& paramset : paramHints)
		{
			evt.signal_id = paramset.model_id;
			evt.event_code = glucose::NDevice_Event_Code::Parameters_Hint;
			//evt.logical_time = logicalTime;
			evt.parameters = paramset.params;
			evt.segment_id = currentSegmentId;

			if (mOutput->send(&evt) != S_OK)
				break;

			logicalTime++;
		}

		bool isError = false;

		do
		{
			// rsSelect_Timesegment_Values_Filter
			// "select measuredat, blood, ist, isig, insulin, carbohydrates, calibration from measuredvalue where segmentid = ? order by measuredat asc"
			//         0           1      2    3     4        5              6

			auto dt = mValueQuery[idx]->value(0).toDateTime();

			double jdate = Unix_Time_To_Rat_Time(dt.toSecsSinceEpoch());

			// go through all value columns
			for (NColumn_Pos i = NColumn_Pos::_Begin; i < NColumn_Pos::_End; ++i)
			{
				auto column = mValueQuery[idx]->value(static_cast<int>(i));

				// if no value is present, skip
				if (column.isNull())
					continue;

				evt.level = column.toDouble(&ok);
				if (!ok)
					continue;

				evt.device_id = Db_Reader_Device_GUID;
				evt.signal_id = ColumnSignalMap[i];
				evt.device_time = jdate + dateCorrection; // apply base correction
				evt.event_code = (i == NColumn_Pos::Calibration) ? glucose::NDevice_Event_Code::Calibrated : glucose::NDevice_Event_Code::Level;
				//evt.logical_time = logicalTime;
				evt.segment_id = currentSegmentId;

				logicalTime++;

				// this may block if the pipe is full (i.e. due to artificial slowdown filter, simulation stepping, etc.)
				if (mOutput->send(&evt) != S_OK)
				{
					isError = true;
					break;
				}
			}
		} while (!isError && mValueQuery[idx]->next());

		// evt.device_time is now guaranteed to have valid time of last sent event

		Send_Segment_Marker(glucose::NDevice_Event_Code::Time_Segment_Stop, evt.device_time, logicalTime, currentSegmentId);
		logicalTime++;

		lastDate = evt.device_time;
	}
}

void CDb_Reader::Run_Main()
{
	glucose::TDevice_Event evt;

	while (mInput->receive(&evt) == S_OK)
	{
		// just fall through in main filter thread
		// there also may be some control code handling (i.e. pausing value sending, etc.)

		if (mOutput->send(&evt) != S_OK)
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
		return E_FAIL;

	mDb = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase(QString::fromStdWString(mDbProvider), mDb_Connection_Name));
	mDb->setHostName(QString::fromStdWString(mDbHost));
	if (mDbPort != 0)
		mDb->setPort(mDbPort);
	mDb->setDatabaseName(QString::fromStdWString(mDbDatabaseName));
	mDb->setUserName(QString::fromStdWString(mDbUsername));
	mDb->setPassword(QString::fromStdWString(mDbPassword));

	if (mDb->open())
	{
		size_t segIdx = 0;
		mValueQuery.resize(mDbTimeSegmentIds.size());

		for (int64_t segId : mDbTimeSegmentIds)
		{
			mValueQuery[segIdx] = std::make_unique<QSqlQuery>(*mDb.get());
			mValueQuery[segIdx]->prepare(rsSelect_Timesegment_Values_Filter);
			mValueQuery[segIdx]->bindValue(0, mDbTimeSegmentIds[segIdx]);
			if (!mValueQuery[segIdx]->exec() || mValueQuery[segIdx]->size() == 0)
			{
				qDebug() << mValueQuery[segIdx]->lastError();
				return E_FAIL;
			}

			segIdx++;
		}

		mReaderThread = std::make_unique<std::thread>(&CDb_Reader::Run_Reader, this);
		Run_Main();
	}
	else
	{
		qDebug() << mDb->lastError();

		return E_FAIL;
	}

	return S_OK;
}

void CDb_Reader::Prepare_Model_Parameters_For(int64_t segmentId, std::vector<StoredModelParams> &paramsTarget) {

	for (const auto &descriptor : glucose::get_model_descriptors()) {
		std::string qry = rsSelect_Params_Base;

		bool first = true;
		for (size_t i = 0; i < descriptor.number_of_parameters; i++)
		{
			if (first)
				first = false;
			else
				qry += ", ";

			qry += std::string(descriptor.parameter_db_column_names[i], descriptor.parameter_db_column_names[i] + wcslen(descriptor.parameter_db_column_names[i]));
		}

		qry += " ";
		qry += rsSelect_Params_From;
		qry += std::string(descriptor.db_table_name, descriptor.db_table_name + wcslen(descriptor.db_table_name)) + " ";
		qry += rsSelect_Params_Condition;

		auto qr = std::make_unique<QSqlQuery>(*mDb.get());

		qr->prepare(qry.c_str());
		qr->bindValue(0, segmentId);
		if (qr->exec() && qr->size() != 0)
		{
			qr->seek(0);

			std::vector<double> arr(descriptor.number_of_parameters);
			for (size_t i = 0; i < descriptor.number_of_parameters; i++)
				arr[i] = qr->value((int)i).toDouble();

			paramsTarget.push_back({ descriptor.id, refcnt::Create_Container<double>(arr.data(), arr.data() + arr.size()) });
		}
	}
}
