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

#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <map>
#include <mutex>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for calculating error metrics
 */
class CSignal_Error : public virtual scgms::CBase_Filter, public virtual scgms::ISignal_Error_Inspection {
protected:
	GUID mReference_Signal_ID = Invalid_GUID;
	GUID mError_Signal_ID = Invalid_GUID;

	std::wstring mDescription;

	std::mutex mSeries_Gaurd;

    struct TSegment_Signals {
        scgms::SSignal reference_signal{ scgms::STime_Segment{}, scgms::signal_BG };
        scgms::SSignal error_signal{ scgms::STime_Segment{}, scgms::signal_BG };
    };
    std::map<uint64_t, TSegment_Signals> mSignal_Series;

	scgms::SMetric mMetric;

	double *mPromised_Metric = nullptr;
    uint64_t mPromised_Segment_id = scgms::All_Segments_Id;
	std::atomic<bool> mNew_Data_Available{false};	


    bool mEmit_Metric_As_Signal = false;
    bool mEmit_Last_Value_Only = false;
    double mLast_Emmitted_Time = std::numeric_limits<double>::quiet_NaN();

    void Emit_Metric_Signal(uint64_t segment_id);

	bool Prepare_Levels(const uint64_t segment_id, std::vector<double> &times, std::vector<double> &reference, std::vector<double> &error);
	double Calculate_Metric(const uint64_t segment_id);	//returns metric or NaN if could not calculate
protected:			
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
    CSignal_Error(scgms::IFilter *output);
	virtual ~CSignal_Error();

	virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;
	virtual HRESULT IfaceCalling Promise_Metric(const uint64_t segment_id, double* const metric_value, bool defer_to_dtor) override final;
	virtual HRESULT IfaceCalling Peek_New_Data_Available() override final;
	virtual HRESULT IfaceCalling Calculate_Signal_Error(const uint64_t segment_id, scgms::TSignal_Stats *absolute_error, scgms::TSignal_Stats *relative_error) override final;
	virtual HRESULT IfaceCalling Get_Description(wchar_t** const desc) override final;
};


#pragma warning( pop )
