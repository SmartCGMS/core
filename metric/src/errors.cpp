#include "errors.h"

#include "../../../common/iface/SolverIface.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/FilterLib.h"

#include "../../../common/lang/dstrings.h"
#include "descriptor.h"

#include <iostream>
#include <chrono>


CErrors_Filter::CErrors_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{inpipe}, mOutput{outpipe} {
	//
}

HRESULT IfaceCalling CErrors_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	
	if (Internal_Query_Interface<glucose::IFilter>(glucose::Error_Filter, *riid, ppvObj)) return S_OK;
	if (Internal_Query_Interface<glucose::IError_Filter_Inspection>(glucose::Error_Filter_Inspection, *riid, ppvObj)) return S_OK;
	
	return E_NOINTERFACE;
}

HRESULT CErrors_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) {
	glucose::SDevice_Event evt;

	mErrorCounter = std::make_unique<CError_Marker_Counter>();
	
	bool updated = false;

	while (mInput.Receive(evt))
	{
		// TODO: set updated flag after bunch of levels came, etc.
		//		 for now, we update error metrics just with segment end

		switch (evt.event_code)
		{
			case glucose::NDevice_Event_Code::Level:
			case glucose::NDevice_Event_Code::Calibrated:
				if (mErrorCounter->Add_Level(evt.segment_id, evt.signal_id, evt.device_time, evt.level))
					updated = mErrorCounter->Recalculate_Errors();
				break;
			case glucose::NDevice_Event_Code::Parameters:
				break;
			case glucose::NDevice_Event_Code::Time_Segment_Stop:
				updated = mErrorCounter->Recalculate_Errors();
				break;
			case glucose::NDevice_Event_Code::Information:
				if (refcnt::WChar_Container_Equals_WString(evt.info.get(), rsSegment_Recalculate_Complete))
					updated = mErrorCounter->Recalculate_Errors_For(evt.signal_id);
				else if (refcnt::WChar_Container_Equals_WString(evt.info.get(), rsParameters_Reset))
					mErrorCounter->Reset_Segment(evt.segment_id, evt.signal_id);
				break;
		}

		if (!mOutput.Send(evt))
			break;

		if (updated)
		{
			glucose::SDevice_Event errEvt{ glucose::NDevice_Event_Code::Information };
			errEvt.info = refcnt::WString_To_WChar_Container_shared(rsInfo_Error_Metrics_Ready);
			mOutput.Send(errEvt);
			updated = false;
		}
	}

	return S_OK;
}


HRESULT IfaceCalling CErrors_Filter::Get_Errors(const GUID *signal_id, const glucose::NError_Type type, glucose::TError_Markers *markers) {
	if (!mErrorCounter)
		return E_FAIL;

	return mErrorCounter->Get_Errors(*signal_id, type, *markers);
}
