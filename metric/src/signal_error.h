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

#include <set>
#include <mutex>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for calculating error metrics
 */
class CSignal_Error : public glucose::CBase_Filter, public virtual glucose::ISignal_Error_Inspection {
protected:
	GUID mReference_Signal_ID = Invalid_GUID;
	GUID mError_Signal_ID = Invalid_GUID;

	std::wstring mDescription;

	std::mutex mSeries_Gaurd;

	/*struct TSignal_Point { double level; double date_time; };
	std::function<bool(TSignal_Point &, TSignal_Point &)> mPair_Comparator = [](const TSignal_Point &lhs, const  TSignal_Point &rhs) {return lhs.date_time < rhs.date_time; };
	using TSignal_Series = std::set<TSignal_Point, decltype(mPair_Comparator)>;
	TSignal_Series mReference_Signal;
	*/
	glucose::SSignal mReference_Signal{ glucose::STime_Segment{}, glucose::signal_BG };
	glucose::SSignal mError_Signal{ glucose::STime_Segment{}, glucose::signal_BG };

	glucose::SMetric mMetric;

	double *mPromised_Metric = nullptr;
	std::atomic<bool> mNew_Data_Available{false};	

	bool Prepare_Levels(std::vector<double> &times, std::vector<double> &reference, std::vector<double> &error);
	double Calculate_Metric();	//returns metric or NaN if could not calculate
protected:			
	virtual HRESULT Do_Execute(glucose::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(glucose::SFilter_Configuration configuration) override final;
public:
	CSignal_Error(glucose::IFilter *output);
	virtual ~CSignal_Error();

	virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;
	virtual HRESULT IfaceCalling Promise_Metric(double* const metric_value, bool defer_to_dtor) override final;
	virtual HRESULT IfaceCalling Peek_New_Data_Available() override final;
	virtual HRESULT IfaceCalling Calculate_Signal_Error(glucose::TSignal_Error *absolute_error, glucose::TSignal_Error *relative_error) override final;
	virtual HRESULT IfaceCalling Get_Description(wchar_t** const desc) override final;
};


#pragma warning( pop )
