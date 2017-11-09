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

HRESULT CFilter_Pipe::send(const glucose::TSensor_Event *event) {
	mQueue.push(*event); 
	return S_OK;
}

HRESULT CFilter_Pipe::receive(glucose::TSensor_Event *event) {
	mQueue.pop(*event);
	return S_OK;
}