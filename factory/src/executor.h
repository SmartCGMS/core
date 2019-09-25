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


#include <thread>
#include <mutex>

class CFilter_Executor : public virtual glucose::IFilter, public virtual refcnt::CReferenced {
protected:	
	glucose::SFilter_Communicator mCommunicator;
	glucose::SFilter mFilter;
	glucose::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;
public:
	CFilter_Executor(const GUID filter_id, glucose::SFilter_Communicator communicator, glucose::IFilter *next_filter, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data);
	virtual ~CFilter_Executor() {};

	virtual HRESULT IfaceCalling Configure(glucose::IFilter_Configuration* configuration) override final;
	virtual HRESULT IfaceCalling Execute(glucose::IDevice_Event *event) override final;
};


class CTerminal_Filter : public virtual glucose::IFilter, public virtual refcnt::CNotReferenced {
	//executer designed to consume events only and to signal the shutdown event
protected:
	glucose::SFilter_Communicator mCommunicator;
	bool mShutdown_Received = false;
public:	
	void Set_Communicator(glucose::SFilter_Communicator communicator);
	virtual ~CTerminal_Filter() {};	

	void Wait_For_Shutdown();	//blocking wait, until it receives the shutdown event

	virtual HRESULT IfaceCalling Configure(glucose::IFilter_Configuration* configuration) override final;
	virtual HRESULT IfaceCalling Execute(glucose::IDevice_Event *event) override final;
};