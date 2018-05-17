#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <tbb/concurrent_queue.h>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CFilter_Pipe :  public glucose::IFilter_Pipe, public virtual refcnt::CReferenced {
protected:
	tbb::concurrent_bounded_queue<glucose::IDevice_Event*> mQueue;
	const std::ptrdiff_t mDefault_Capacity = 64;
	bool mShutting_Down_Send = false;
	bool mShutting_Down_Receive = false;
		//we need two flags to let the last shutdown event to fall through
public:
	CFilter_Pipe() noexcept;
	virtual ~CFilter_Pipe();

	virtual HRESULT send(glucose::IDevice_Event* event) override final;
	virtual HRESULT receive(glucose::IDevice_Event** event) override final;
	virtual HRESULT abort() final;
};

#pragma warning( pop )

#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe);
#else
	extern "C" HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe);
#endif
