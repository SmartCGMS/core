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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/DbLib.h>

#include <memory>
#include <thread>
#include <vector>

struct TStored_Model_Params_Legacy {
	const GUID model_id;
	scgms::SModel_Parameter_Vector params;
};


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Class that reads selected segments from the db produces the events
 * i.e., it mimicks CGMS
 */
class CDb_Reader_Legacy : public scgms::CBase_Filter, public db::IDb_Sink{
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
		// loaded segments ID
		std::vector<int64_t> mDbTimeSegmentIds;
		// do we need to send shutdown after last value?
		bool mShutdownAfterLast = false;

		db::SDb_Connector mDb_Connector;
		db::SDb_Connection mDb_Connection;

		std::atomic<bool> mQuit_Flag{ false };
		// reader thread
		std::unique_ptr<std::thread> mDb_Reader_Thread;

	protected:
		void Db_Reader();
		void End_Db_Reader();
	
		bool Emit_Segment_Marker(scgms::NDevice_Event_Code code, int64_t segment_id);
		bool Emit_Segment_Parameters(int64_t segment_id);
		bool Emit_Segment_Levels(int64_t segment_id);
		bool Emit_Shut_Down();

		bool Emit_Info_Event(const std::wstring& info);

	protected:
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CDb_Reader_Legacy(scgms::IFilter *output);
		virtual ~CDb_Reader_Legacy();

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;

		virtual HRESULT IfaceCalling Set_Connector(db::IDb_Connector *connector) override;
};

#pragma warning( pop )
