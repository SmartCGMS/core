#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <cstdint>
#include <memory>
#include <thread>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

// class used as net communicator - may as as receiving or sending side
class CNet_Comm : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		// configured host
		std::string mHost;
		// configured port
		uint16_t mPort;
		// is this receiving side?
		bool mRecvSide;

		// starts and runs receiving side (ignores input pipe)
		void Run_Recv();
		// starts and runs sending side (ignores output pipe)
		void Run_Send();

	public:
		CNet_Comm(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
};

#pragma warning( pop )
