#include "pipe.h"

#include "../../../common/rtl/manufactory.h"

HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe) {
	return Manufacture_Object<CFilter_Pipe, glucose::IFilter_Pipe>(pipe);
}

CFilter_Pipe::CFilter_Pipe() {
	mQueue.set_capacity(mDefault_Capacity);
}

CFilter_Pipe::~CFilter_Pipe() {
	//just empty to call all the dctors correctly
}

HRESULT CFilter_Pipe::send(const glucose::TDevice_Event* event) {

	if (mShutting_Down_Send)
		return S_FALSE;

	if (event->event_code == glucose::NDevice_Event_Code::Shut_Down)
		mShutting_Down_Send = true;

	try {
		mQueue.push(*event);
	}
	catch (tbb::user_abort &) {
		mShutting_Down_Send = true;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::receive(glucose::TDevice_Event* const event) {

	if (mShutting_Down_Receive)
		return S_FALSE;

	try {
		mQueue.pop(*event);

		if (event->event_code == glucose::NDevice_Event_Code::Shut_Down)
			mShutting_Down_Receive = true;

	}
	catch (tbb::user_abort &) {
		mShutting_Down_Receive = true;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::abort() {
	//TODO: https://stackoverflow.com/questions/16961990/thread-building-blocks-concurrent-bounded-queue-how-do-i-close-it?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
	mShutting_Down_Send = mShutting_Down_Receive = true;
	mQueue.abort();
	return S_OK;
}
