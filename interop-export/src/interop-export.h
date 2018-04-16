#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <memory>
#include <thread>
#include <cstdint>

using TInterop_Listener = void(*)(uint8_t /*code*/, uint8_t* /*device guid*/, uint8_t* /*signal guid*/,
	int64_t /*logical time*/, float /*device time*/, float /*value*/, glucose::IModel_Parameter_Vector* /*params*/,
	refcnt::wstr_container* /* info */);

/*
 * Filter class for computing model parameters
 */
class CInterop_Export_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::IFilter_Pipe* mInput;
		// output pipe
		glucose::IFilter_Pipe* mOutput;

		int64_t mLogicalTime;

		// thread function
		void Run_Main();

	public:
		CInterop_Export_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe);
		virtual ~CInterop_Export_Filter();

		virtual HRESULT Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) override final;
};

// register listener for events
extern "C" void Register_Listener(TInterop_Listener listener);
