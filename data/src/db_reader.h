#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DbLib.h"

#include <memory>
#include <thread>
#include <vector>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlQueryModel>

struct TStored_Model_Params {
	const GUID model_id;
	glucose::SModel_Parameter_Vector params;
};


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Class that reads selected segments from the db produces the events
 * i.e., it mimicks CGMS
 */
class CDb_Reader : public virtual glucose::IFilter, public virtual db::IDb_Sink, public virtual refcnt::CReferenced {
	protected:
		glucose::SFilter_Pipe mInput;
		glucose::SFilter_Pipe mOutput;

		// database host configured
		std::wstring mDbHost;
		// configured DB port
		uint16_t mDbPort;
		// database provider string in Qt format
		std::wstring mDbProvider;
		// database name / filename
		std::wstring mDbDatabaseName;
		// username to be used
		std::wstring mDbUsername;
		// password to be used
		std::wstring mDbPassword;
		// loaded segments ID
		std::vector<int64_t> mDbTimeSegmentIds;
		// do we need to send shutdown after last value?
		bool mShutdownAfterLast = false;

	protected:
		std::atomic<bool> mQuit_Flag{ false };
		// reader thread
		std::unique_ptr<std::thread> mDb_Reader_Thread;
		void Db_Reader();
		void End_Db_Reader();
		bool Configure(glucose::SFilter_Parameters configuration);
	
		bool Emit_Segment_Marker(glucose::NDevice_Event_Code code, int64_t segment_id);
		bool Emit_Segment_Parameters(int64_t segment_id);
		bool Emit_Segment_Levels(int64_t segment_id);
		bool Emit_Shut_Down();

	protected:
		db::SDb_Connector mDb_Connector;
		db::SDb_Connection mDb_Connection;

	public:
		CDb_Reader(glucose::SFilter_Pipe in_pipe, glucose::SFilter_Pipe out_pipe);
		virtual ~CDb_Reader() {};

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;
		virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration *configuration) override;
		virtual HRESULT IfaceCalling Set_Connector(db::IDb_Connector *connector) override;
};

#pragma warning( pop )
