#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <tbb/concurrent_queue.h>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CFilter_Pipe :  public glucose::IFilter_Pipe, public virtual refcnt::CReferenced {
protected:
	tbb::concurrent_bounded_queue<glucose::TDevice_Event> mQueue;
	const std::ptrdiff_t mDefault_Capacity = 64;
	bool mAborted = false;
public:
	CFilter_Pipe();
	virtual ~CFilter_Pipe();

	virtual HRESULT send(const glucose::TDevice_Event* event);
	virtual HRESULT receive(glucose::TDevice_Event* const event);
	virtual HRESULT abort() final;
};

#pragma warning( pop )

#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe);
#else
	extern "C" HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe);
#endif
