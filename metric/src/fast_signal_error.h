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
 * So far, it calcules avg+sd only. In the future, it could be extended.
 */
class CFast_Signal_Error : public virtual scgms::CBase_Filter, public virtual scgms::ILogical_Clock, public virtual scgms::ISignal_Error_Inspection {
protected:
	//this protected section has to be moved to a separate class, once we support more metrics
	
	double mAccumulator = 0.0;
	double mVariance = 0.0;
	
	double avg_divisor();
	void Update_Counters(const double difference);
	double Calculate_Metric();
	void Clear_Counters();
protected:
	double mLevels_Counter = 0.0;

	struct TSignal_Info {
		double level;
		double device_time;
		double slope;
	};

	enum class NSignal_Id : size_t {
		reference = 0,
		error,
		count
	};

	std::array<TSignal_Info, static_cast<size_t>(NSignal_Id::count)> mSignals{};

	void Update_Signal_Info(const double level, const double device_time, const bool reference_signal);
	void Clear_Signal_Info();
protected:
	GUID mReference_Signal_ID = Invalid_GUID;
	GUID mError_Signal_ID = Invalid_GUID;
	
	std::wstring mDescription;

	bool mRelative_Error = true;
	bool mPrefer_More_Levels = false;
	bool mSquared_Diff = false;
	size_t mLevels_Required = 0;

	std::atomic<ULONG> mNew_Data_Logical_Clock{ 0 };

	double* mPromised_Metric = nullptr;
protected:
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override;
public:
	CFast_Signal_Error(scgms::IFilter *output);
	virtual ~CFast_Signal_Error();
	

	virtual HRESULT IfaceCalling QueryInterface(const GUID* riid, void** ppvObj) override final;
	virtual HRESULT IfaceCalling Promise_Metric(const uint64_t segment_id, double* const metric_value, BOOL defer_to_dtor) override final;
	virtual HRESULT IfaceCalling Calculate_Signal_Error(const uint64_t segment_id, scgms::TSignal_Stats* absolute_error, scgms::TSignal_Stats* relative_error) override final;
	virtual HRESULT IfaceCalling Get_Description(wchar_t** const desc) override;
	virtual HRESULT IfaceCalling Logical_Clock(ULONG* clock) override final;
};


#pragma warning( pop )
