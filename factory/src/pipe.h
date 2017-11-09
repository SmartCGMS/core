#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <tbb/concurrent_queue.h>


class CFilter_Pipe :  public glucose::IFilter_Pipe, public virtual refcnt::CReferenced {
protected:
	tbb::concurrent_bounded_queue<glucose::TSensor_Event> mQueue;
	const std::ptrdiff_t mDefault_Capacity = 64;
public:	
	CFilter_Pipe();
	virtual ~CFilter_Pipe();

	virtual HRESULT send(const glucose::TSensor_Event *event);
	virtual HRESULT receive(glucose::TSensor_Event *event);
};



#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe);
#endif