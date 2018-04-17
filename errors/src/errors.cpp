#include "errors.h"

#include "../../../common/iface/SolverIface.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/FilterLib.h"

#include "../../../common/lang/dstrings.h"
#include "../../factory/src/filters.h"

#include "../../metric/src/descriptor.h"

#include <iostream>
#include <chrono>

std::atomic<CErrors_Filter*> CErrors_Filter::mInstance = nullptr;

CErrors_Filter::CErrors_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
	: mInput(inpipe), mOutput(outpipe)
{
	//
}

void CErrors_Filter::Run_Main()
{
	glucose::TDevice_Event evt;

	mErrorCounter = std::make_unique<CError_Metric_Counter>();
	
	bool updated = false;

	while (mInput->receive(&evt) == S_OK)
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
				if (refcnt::WChar_Container_Equals_WString(evt.info, rsSegment_Recalculate_Complete))
					updated = mErrorCounter->Recalculate_Errors_For(evt.signal_id);
				else if (refcnt::WChar_Container_Equals_WString(evt.info, rsParameters_Reset))
					mErrorCounter->Reset_Segment(evt.segment_id, evt.signal_id);
				break;
		}

		if (mOutput->send(&evt) != S_OK)
			break;

		if (updated)
		{
			glucose::TDevice_Event errEvt;

			errEvt.event_code = glucose::NDevice_Event_Code::Information;
			errEvt.device_time = Unix_Time_To_Rat_Time(time(nullptr));
			errEvt.info = refcnt::WString_To_WChar_Container(rsInfo_Error_Metrics_Ready);
			//errEvt.logical_time = 0;
			errEvt.signal_id = { 0 };
			errEvt.device_id = { 0 };

			mOutput->send(&errEvt);
			updated = false;
		}
	}

	// release instance
	mInstance = nullptr;
}

CErrors_Filter* CErrors_Filter::Get_Instance()
{
	return mInstance;
}

HRESULT CErrors_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	CErrors_Filter* instance = nullptr;
	if (!mInstance.compare_exchange_strong(instance, this))
	{
		std::wcerr << L"More than one instance of error metrics filter initialized!" << std::endl;
		return E_FAIL;
	}

	Run_Main();

	return S_OK;
}

HRESULT CErrors_Filter::Get_Errors(const GUID* signal_id, glucose::TError_Container* target, glucose::NError_Type type)
{
	if (!mErrorCounter)
		return E_FAIL;;

	return mErrorCounter->Get_Errors(signal_id, target, type);
}

extern "C" HRESULT IfaceCalling get_error_metrics(const GUID* signal_id, glucose::TError_Container* target, glucose::NError_Type type)
{
	CErrors_Filter* flt = CErrors_Filter::Get_Instance();
	if (!flt)
		return E_FAIL;

	return flt->Get_Errors(signal_id, target, type);
}
