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

#include <scgms/rtl/DeviceLib.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/SolverLib.h>
#include <scgms/rtl/referencedImpl.h>

#include <map>
#include <mutex>
#include <fstream>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * base class for comparig two signals
 */
class CTwo_Signals : public virtual scgms::CBase_Filter, public virtual scgms::ILogical_Clock {
	protected:
		GUID mReference_Signal_ID = Invalid_GUID;
		GUID mError_Signal_ID = Invalid_GUID;
		filesystem::path mCSV_Path;

		std::wstring mDescription;

		std::mutex mSeries_Gaurd;

		struct TSegment_Signals {
			scgms::SSignal reference_signal{ scgms::STime_Segment{}, scgms::signal_BG };
			scgms::SSignal error_signal{ scgms::STime_Segment{}, scgms::signal_BG };
			bool last_value_emitted = false;        //fixing for logs, which do not contain proper segment start stop marks
		};
		std::map<uint64_t, TSegment_Signals> mSignal_Series;

		std::atomic<ULONG> mNew_Data_Logical_Clock{0};

		bool mShutdown_Received = false;

	protected:
		bool Prepare_Levels(const uint64_t segment_id, std::vector<double>& times, std::vector<double>& reference, std::vector<double>& error);
		bool Prepare_Unaligned_Discrete_Levels(const uint64_t segment_id, std::vector<double>& times, std::vector<double>& reference, std::vector<double>& error_times, std::vector<double>& error, bool allow_multipoint_affinity = false);

		virtual HRESULT On_Level_Added(const uint64_t segment_id, const double device_time) {
			return S_OK;
		}

		void Flush_Stats();

		virtual void Do_Flush_Stats(std::wofstream stats_file) = 0;

		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override;

	public:
		CTwo_Signals(scgms::IFilter *output);
		virtual ~CTwo_Signals();
	
		virtual HRESULT IfaceCalling Logical_Clock(ULONG *clock) override final;
		virtual HRESULT IfaceCalling Get_Description(wchar_t** const desc);
};


#pragma warning( pop )
