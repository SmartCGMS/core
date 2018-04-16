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

	if (mAborted)
		return S_FALSE;

	try {
		mQueue.push(*event);
	}
	catch (tbb::user_abort &) {
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::receive(glucose::TDevice_Event* const event) {

	if (mAborted)
		return S_FALSE;

	try {
		mQueue.pop(*event);
	}
	catch (tbb::user_abort &) {
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::abort() {
	mAborted = true;
	mQueue.abort();
	return S_OK;
}
