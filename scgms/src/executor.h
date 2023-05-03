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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include "../../../common/rtl/FilterLib.h"

#include "device_event.h"

#include <mutex>
#include <condition_variable>


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CFilter_Executor : public virtual scgms::IFilter, public virtual refcnt::CNotReferenced {
protected:
	std::recursive_mutex &mCommunication_Guard;
	scgms::SFilter mFilter;
	scgms::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;
public:
	CFilter_Executor(const GUID filter_id, std::recursive_mutex &communication_guard, scgms::IFilter *next_filter, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data);
	virtual ~CFilter_Executor() = default;

	void Release_Filter();

	virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;

	virtual HRESULT IfaceCalling Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list *error_description) override final;
	virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event) override final;
};

class CTerminal_Filter : public virtual scgms::IFilter, public virtual refcnt::CNotReferenced {
	//executer designed to consume events only and to signal the shutdown event
protected:
	std::mutex mShutdown_Guard;
	std::condition_variable mShutdown_Condition;
	bool mShutdown_Received = false;
	scgms::IFilter *mCustom_Output = nullptr;
public:
	CTerminal_Filter(scgms::IFilter *custom_output);
	virtual ~CTerminal_Filter() = default;

	void Wait_For_Shutdown();	//blocking wait, until it receives the shutdown event

	virtual HRESULT IfaceCalling Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list* error_description) override final;
	virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event) override;
};

class CCopying_Terminal_Filter : public virtual CTerminal_Filter {
protected:
	std::vector<CDevice_Event> &mEvents;
	bool mDo_Not_Copy_Info_Events = true;
public:
	CCopying_Terminal_Filter(std::vector<CDevice_Event> &events, bool do_not_copy_info_events);
	virtual ~CCopying_Terminal_Filter() = default;
	virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event) override final;
};

#pragma warning( pop )
