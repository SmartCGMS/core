#include "executor.h"
#include "filters.h"

CFilter_Executor::CFilter_Executor(glucose::IFilter_Executor *consument) : mConsument{consument} {
	mQueue.set_capacity(mDefault_Capacity);
}


HRESULT IfaceCalling CFilter_Executor::receive(glucose::IDevice_Event **event) {
	if (mShutting_Down_Receive)
		return S_FALSE;

	try {
		mQueue.pop(*event);

		glucose::TDevice_Event *raw_event;
		if ((*event)->Raw(&raw_event) == S_OK)	//should be OK since we call it in ::send
			mShutting_Down_Receive = raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down;
	}

	catch (tbb::user_abort &) {
		mShutting_Down_Receive = true;
		return S_FALSE;
	}
	return S_OK;

}

HRESULT IfaceCalling CFilter_Executor::send(glucose::IDevice_Event *event) {
	return mConsument->push_back(event);
}


HRESULT IfaceCalling CFilter_Executor::push_back(glucose::IDevice_Event *event) {
	if (event == nullptr)
		return E_INVALIDARG;

	if (mShutting_Down_push_back)
		return S_FALSE;

	glucose::TDevice_Event *raw_event;
	HRESULT rc = event->Raw(&raw_event);
	if (rc != S_OK) return rc;

	if (raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down)
		mShutting_Down_push_back = true;

	try {
		mQueue.push(event);
	}
	catch (tbb::user_abort &) {
		mShutting_Down_push_back = true;
		return S_FALSE;
	}
	return S_OK;
}

void CFilter_Executor::abort() {
	mShutting_Down_push_back = mShutting_Down_Receive = true;
	mQueue.abort();
	mQueue.clear();
}



CAsync_Filter_Executor::CAsync_Filter_Executor(const GUID filter_id, glucose::IFilter_Configuration *configuration, glucose::IFilter_Executor *consument, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) : CFilter_Executor(consument)  {
	receiver ani send nemuzou byt this, jinak to udela kruhovou zavislost
		to same pro sync filtr

		metody receive, pushback a send musi jit do samostatne tridy, ktera pak pujde do samostatneho objektu a tudiz nevznikne kruhova zavislost

	mFilter = create_filter(filter_id, static_cast<glucose::IEvent_Receiver*>(this), static_cast<glucose::IEvent_Sender*>(this));	
	if (!SUCCEEDED(mFilter->Configure(configuration))) throw std::invalid_argument::invalid_argument("Cannot configure the filter!");
	//at this point, we will call a callback function to perform any additional configuration of the filter we've just created 
	on_filter_created(mFilter.get(), on_filter_created_data);
	//once configured, do not execute yet - do this in the start method	
}

void CAsync_Filter_Executor::start() {
	abort();
	join();
	mShutting_Down_push_back = mShutting_Down_Receive = false;
	mThread = std::make_unique<std::thread>([this]() {	mFilter->Execute();	});
}


void CAsync_Filter_Executor::join() {	
	if (mThread)	
		if (mThread->joinable()) 
			mThread->join();
}


CSync_Filter_Executor::CSync_Filter_Executor(const GUID filter_id, glucose::IFilter_Configuration *configuration, glucose::IFilter_Executor *consument, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) : CFilter_Executor(consument) {
	mFilter = create_filter(filter_id, static_cast<glucose::IEvent_Receiver*>(this), static_cast<glucose::IEvent_Sender*>(this));
	if (!SUCCEEDED(mFilter->Configure(configuration))) throw std::invalid_argument::invalid_argument("Cannot configure the filter!");
	//at this point, we will call a callback function to perform any additional configuration of the filter we've just created 
	on_filter_created(mFilter.get(), on_filter_created_data);
	//once configured, we can call its execute method later on
}



HRESULT IfaceCalling CSync_Filter_Executor::push_back(glucose::IDevice_Event *event) {
	if (!mFilter) return E_FAIL;
	HRESULT rc = CFilter_Executor::push_back(event);
	if (rc == S_OK)
		rc = mFilter->Execute();
	return rc;	
}

CCopy_Event_Executor::CCopy_Event_Executor(glucose::SEvent_Sender output) : mOutput(output) {
	//
};

HRESULT IfaceCalling CCopy_Event_Executor::push_back(glucose::IDevice_Event *event) { 
	return mOutput->send(event); 
};

HRESULT IfaceCalling CTerminal_Executor::push_back(glucose::IDevice_Event *event) { 
	event->Release(); return S_OK; 
};