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
#include "event_communicator.h"

#include <tbb/concurrent_queue.h>

#include <thread>
#include <mutex>

class CExecutor : public virtual glucose::IFilter_Communicator, public virtual refcnt::CReferenced {
public:
	virtual ~CExecutor() {}
		
	virtual void start() {};
	virtual void join() {};
	virtual void abort() = 0; //terminate and join
};

class CFilter_Executor : public virtual CExecutor, public virtual glucose::IEvent_Receiver, public virtual glucose::IEvent_Sender {
protected:
	refcnt::SReferenced<glucose::IFilter_Communicator> mConsument;
	tbb::concurrent_bounded_queue<glucose::IDevice_Event*> mQueue;
	const std::ptrdiff_t mDefault_Capacity = 64;	
	bool mShutting_Down_push_back = false;
	bool mShutting_Down_Receive = false;
	//we need two flags to let the last shutdown event to fall through
	glucose::SFilter mFilter;
public:
	CFilter_Executor(glucose::IFilter_Communicator *consument);
	virtual ~CFilter_Executor() {}

	virtual HRESULT IfaceCalling receive(glucose::IDevice_Event **event) override;
	virtual HRESULT IfaceCalling send(glucose::IDevice_Event *event) override;
	virtual HRESULT IfaceCalling push_back(glucose::IDevice_Event *event) override;

	virtual void abort() override final;
};

class CAsync_Filter_Executor : public virtual CFilter_Executor {
protected:
	 std::unique_ptr<std::thread> mThread;
public:
	CAsync_Filter_Executor(const GUID filter_id, glucose::IFilter_Configuration *configuration, glucose::IFilter_Communicator *consument, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data);
	virtual ~CAsync_Filter_Executor() {};	

	virtual void start() override final;
	virtual void join() override final;
};

class CSync_Filter_Executor : public virtual CExecutor {
protected:
	CEvent_Communicator mCommunicator;
	std::mutex &mConsumer_Guard;
	glucose::SFilter mFilter;
public:
	CSync_Filter_Executor(const GUID filter_id, glucose::IFilter_Configuration *configuration, std::mutex &consumer_gaurd, glucose::IFilter_Communicator *consument, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data);
	virtual ~CSync_Filter_Executor() {};

	virtual HRESULT IfaceCalling push_back(glucose::IDevice_Event *event) override;
};


class CCopy_Event_Executor : public virtual CExecutor {
protected:
	glucose::SEvent_Sender mOutput;
public:
	CCopy_Event_Executor(glucose::SEvent_Sender output);
	virtual ~CCopy_Event_Executor() {};
	virtual void abort() override final {};
	virtual HRESULT IfaceCalling push_back(glucose::IDevice_Event *event) override final;
};

class CTerminal_Executor : public virtual CExecutor {
	//executer designed to consume events only
public:	
	virtual ~CTerminal_Executor() {};
	virtual void abort() override final {};
	virtual HRESULT IfaceCalling push_back(glucose::IDevice_Event *event) override final;
};