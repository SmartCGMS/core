#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/DbLib.h"

#include <memory>
#include <thread>
#include <vector>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlQueryModel>

struct StoredModelParams
{
	const GUID model_id;
	glucose::IModel_Parameter_Vector* params;
};


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Class that reads selected segments from the db produces the events
 * i.e., it mimicks CGMS
 */
class CDb_Reader : public virtual glucose::IFilter, public virtual db::IDb_Sink, public virtual refcnt::CReferenced
{
	protected:
		// input pipe instance
		glucose::IFilter_Pipe* mInput;
		// output pipe instance
		glucose::IFilter_Pipe* mOutput;

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
		// current segment
		int64_t mCurrentSegmentIdx;

		// loaded model parameters of segment currently being sent
		std::map<int64_t, std::vector<StoredModelParams>> mModelParams;

		// prepare model parameters (load from DB) to be sent through output pipe
		void Prepare_Model_Parameters_For(int64_t segmentId, std::vector<StoredModelParams> &paramsTarget);

		// reader thread
		std::unique_ptr<std::thread> mReaderThread;

		// reader thread main method - contains main logic
		void Run_Reader();
		// runs main filter logic (pipes and stuff)
		void Run_Main();

		// sends segment marker through output pipe
		void Send_Segment_Marker(glucose::NDevice_Event_Code code, double device_time, uint64_t segment_id);
	protected:
		db::SDb_Connector mDb_Connector;
		db::SDb_Connection mDb_Connection;

		// performs database query
		QSqlQuery* Get_Segment_Query(db::SDb_Query squery, int64_t segmentId);

	public:
		CDb_Reader(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);
		virtual ~CDb_Reader();

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;
		virtual HRESULT IfaceCalling Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
		virtual HRESULT IfaceCalling Set_Connector(db::IDb_Connector *connector) override final;
};

#pragma warning( pop )
