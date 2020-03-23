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


#include <memory>
#include <thread>
#include <map>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

namespace signal_generator_internal {
	class CSynchronized_Generator : public virtual scgms::IFilter, public virtual refcnt::CNotReferenced {
	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		bool mCatching_Up = false;
		GUID mSync_Signal = Invalid_GUID;		
		double mTime_To_Catch_Up = 0.0;
		double mFixed_Stepping = 5.0*scgms::One_Minute;
		double mLast_Device_Time = std::numeric_limits<double>::quiet_NaN();
		scgms::SDiscrete_Model mSync_Model;

		scgms::IFilter *mDirect_Output;	//output we use if the event was for a single event
		scgms::IFilter *mChained_Output; //output we use if the event was shutdown all for all segments
	public:
		CSynchronized_Generator(scgms::IFilter *direct_output, scgms::IFilter *chained_output, const uint64_t segment_id);
		virtual ~CSynchronized_Generator();

		HRESULT Execute_Sync(scgms::UDevice_Event event);

		virtual HRESULT IfaceCalling Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list* error_description) override final;
		virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event) override final;
	};
}

/*
 * Filter class for generating signals using a specific model 
 */
class CSignal_Generator : public virtual scgms::CBase_Filter, public virtual scgms::IFilter_Feedback_Receiver {
protected:
	bool mSync_To_Signal = false;
	GUID mSync_Signal = Invalid_GUID;
	double mFixed_Stepping = 5.0*scgms::One_Minute;
	double mMax_Time = 24.0 * scgms::One_Hour;			//maximum time, for which the generator can run
	double mTotal_Time = 0.0;	//time for which the generator runs	
	bool mEmit_Shutdown = true;	
	scgms::SDiscrete_Model mAsync_Model;
	scgms::SFilter_Configuration mSync_Configuration;
protected:
	signal_generator_internal::CSynchronized_Generator* mLast_Sync_Generator = nullptr;
	using TSync_Model = std::unique_ptr<signal_generator_internal::CSynchronized_Generator>;
	std::map<uint64_t, TSync_Model> mSync_Models;
protected:
	std::wstring mFeedback_Name;	
	std::unique_ptr<std::thread> mThread;
	bool mQuitting = false;
	void Stop_Generator(bool wait);
protected:
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CSignal_Generator(scgms::IFilter *output);
	virtual ~CSignal_Generator();

	virtual HRESULT IfaceCalling Name(wchar_t** const name) override final;
	virtual HRESULT IfaceCalling QueryInterface(const GUID* riid, void** ppvObj) override;
};

#pragma warning( pop )
