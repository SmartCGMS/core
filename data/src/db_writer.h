#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DbLib.h"

#include <memory>
#include <thread>
#include <vector>
#include <set>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlQueryModel>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

struct CPrepared_Value
{
	double measuredAt;
	GUID signalId;
	double value;
	int64_t segmentId;
};

/*
 * Filter class for writing data and parameters to database
 */
class CDb_Writer : public virtual glucose::IFilter, public virtual db::IDb_Sink, public virtual refcnt::CReferenced
{
	protected:
		glucose::SFilter_Pipe mInput;
		glucose::SFilter_Pipe mOutput;

		// database host configured
		std::wstring mDbHost;
		// configured DB port
		uint16_t mDbPort = 0;
		// database provider string in Qt format
		std::wstring mDbProvider;
		// database name / filename
		std::wstring mDbDatabaseName;
		// username to be used
		std::wstring mDbUsername;
		// password to be used
		std::wstring mDbPassword;
		// subject id to be used
		int64_t mSubject_Id;

		// generate new primary keys for all incoming data intended to be stored?
		bool mGenerate_Primary_Keys;
		// store incoming data (levels, masked levels, ...)?
		bool mStore_Data;
		// store parameters?
		bool mStore_Parameters;

		// mapping of simulator segment IDs to database IDs
		std::map<int64_t, int64_t> mSegment_Db_Id_Map;
		// set of all signals to be ignored when storing to DB
		std::set<GUID> mIgnored_Signals;
		// list of prepared values
		std::list<CPrepared_Value> mPrepared_Values;

		int64_t mLast_Generated_Idx = 0;

	protected:
		db::SDb_Connector mDb_Connector;
		db::SDb_Connection mDb_Connection;

		int64_t Get_Db_Segment_Id(int64_t segment_id);

		// generates a new timesegment, assigns a new id (primary key) and stores into database
		int64_t Create_Segment(std::wstring name, std::wstring comment);
		// generated a new subject
		int64_t Create_Subject(std::wstring name);

		// stores incoming level to database
		bool Store_Level(const glucose::UDevice_Event& evt);
		// stores incoming parameters to database
		bool Store_Parameters(const glucose::UDevice_Event& evt);

		// flushes cached levels to database
		void Flush_Levels();

		bool Configure(glucose::SFilter_Parameters conf);

	public:
		CDb_Writer(glucose::SFilter_Pipe in_pipe, glucose::SFilter_Pipe out_pipe);
		virtual ~CDb_Writer() {};

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;
		virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration *configuration) override;
		virtual HRESULT IfaceCalling Set_Connector(db::IDb_Connector *connector) override;
};

#pragma warning( pop )
