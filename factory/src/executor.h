#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <tbb/concurrent_queue.h>

#include <thread>

class CExecutor : public virtual glucose::IFilter_Executor, public virtual refcnt::CReferenced {
public:
	virtual ~CExecutor() {}
		
	virtual void start() {};
	virtual void join() {};
	virtual void abort() = 0; //terminate and join
};

class CFilter_Executor : public virtual CExecutor, public virtual glucose::IEvent_Receiver, public virtual glucose::IEvent_Sender {
protected:
	refcnt::SReferenced<glucose::IFilter_Executor> mConsument;
	tbb::concurrent_bounded_queue<glucose::IDevice_Event*> mQueue;
	const std::ptrdiff_t mDefault_Capacity = 64;	
	bool mShutting_Down_push_back = false;
	bool mShutting_Down_Receive = false;
	//we need two flags to let the last shutdown event to fall through
	glucose::SFilter mFilter;
public:
	CFilter_Executor(glucose::IFilter_Executor *consument);
	virtual ~CFilter_Executor() {}

	virtual HRESULT IfaceCalling receive(glucose::IDevice_Event **event) override final;
	virtual HRESULT IfaceCalling send(glucose::IDevice_Event *event) override final;
	virtual HRESULT IfaceCalling push_back(glucose::IDevice_Event *event) override;

	virtual void abort() override final;
};

class CAsync_Filter_Executor : public virtual CFilter_Executor {
protected:
	 std::unique_ptr<std::thread> mThread;
public:
	CAsync_Filter_Executor(const GUID filter_id, glucose::IFilter_Configuration *configuration, glucose::IFilter_Executor *consument, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data);
	virtual ~CAsync_Filter_Executor() {};	

	virtual void start() override final;
	virtual void join() override final;
};

class CSync_Filter_Executor : public virtual CFilter_Executor {
protected:

public:
	CSync_Filter_Executor(const GUID filter_id, glucose::IFilter_Configuration *configuration, glucose::IFilter_Executor *consument, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data);
	virtual ~CSync_Filter_Executor() {};

	virtual HRESULT IfaceCalling push_back(glucose::IDevice_Event *event) override final;
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