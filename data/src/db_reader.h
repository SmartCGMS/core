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
class CDb_Reader : public glucose::IAsynchronous_Filter, public db::IDb_Sink, public virtual refcnt::CReferenced {
	protected:
		glucose::SFilter_Asynchronous_Pipe mInput;
		glucose::SFilter_Asynchronous_Pipe mOutput;

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
		CDb_Reader(glucose::SFilter_Asynchronous_Pipe in_pipe, glucose::SFilter_Asynchronous_Pipe out_pipe);
		virtual ~CDb_Reader() {};

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;
		virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration *configuration) override;
		virtual HRESULT IfaceCalling Set_Connector(db::IDb_Connector *connector) override;
};

#pragma warning( pop )
