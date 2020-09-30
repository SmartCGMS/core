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

#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/referencedImpl.h"

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


	bool Prepare_Levels(const uint64_t segment_id, std::vector<double> &times, std::vector<double> &reference, std::vector<double> &error);
	virtual void On_Level_Added(const uint64_t segment_id, const double device_time) {};
	void Flush_Stats();
protected:
	virtual void Do_Flush_Stats(std::wofstream stats_file) = 0;
protected:
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override ;
public:
	CTwo_Signals(scgms::IFilter *output);
	virtual ~CTwo_Signals();
	
	virtual HRESULT IfaceCalling Logical_Clock(ULONG *clock) override final;
	virtual HRESULT IfaceCalling Get_Description(wchar_t** const desc);
};


#pragma warning( pop )
