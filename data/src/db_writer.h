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

#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DbLib.h"

#include <memory>
#include <thread>
#include <vector>
#include <set>
#include <list>

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
class CDb_Writer : public scgms::CBase_Filter, public db::IDb_Sink {
	protected:
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
		int64_t mSubject_Id = db::New_Subject_Identifier;

		// generate new primary keys for all incoming data intended to be stored?
		bool mGenerate_Primary_Keys = false;
		// store incoming data (levels, masked levels, ...)?
		bool mStore_Data = false;
		// store parameters?
		bool mStore_Parameters = false;

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
		bool Store_Level(const scgms::UDevice_Event& evt);
		// stores incoming parameters to database
		bool Store_Parameters(const scgms::UDevice_Event& evt);

		// flushes cached levels to database
		void Flush_Levels();

		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
	public:
		CDb_Writer() noexcept = default;
		virtual ~CDb_Writer();

		TEmbedded_Error Initialize(scgms::IFilter* output) noexcept;		//if HRESULT != S_OK, wchar_t* describes the error

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;		
		virtual HRESULT IfaceCalling Set_Connector(db::IDb_Connector *connector) override;
};

#pragma warning( pop )
