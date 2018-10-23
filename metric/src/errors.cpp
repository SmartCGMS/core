/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8
 * 301 00, Pilsen
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 */

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

HRESULT CErrors_Filter::Run(glucose::IFilter_Configuration* configuration) {
	
	mErrorCounter = std::make_unique<CError_Marker_Counter>();
	
	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
	
		switch (evt.event_code)
		{
			case glucose::NDevice_Event_Code::Level:
			case glucose::NDevice_Event_Code::Masked_Level:
				// the internal logic will tell us, if the signal is reference signal, and therefore we need to recalculate errors
				if (mErrorCounter->Add_Level(evt.segment_id, evt.signal_id, evt.device_time, evt.level))
					mErrorCounter->Recalculate_Errors();
				break;
			case glucose::NDevice_Event_Code::Parameters:
				break;
			case glucose::NDevice_Event_Code::Time_Segment_Stop:
				mErrorCounter->Recalculate_Errors();
				break;
			case glucose::NDevice_Event_Code::Information:
				if (evt.info == rsSegment_Recalculate_Complete)
					mErrorCounter->Recalculate_Errors_For(evt.signal_id);
				else if (evt.info == rsParameters_Reset)
					mErrorCounter->Reset_Segment(evt.segment_id, evt.signal_id);
				break;
			case glucose::NDevice_Event_Code::Warm_Reset:
				mErrorCounter->Reset_All();
				break;
			default:
				break;
		}

		if (!mOutput.Send(evt))
			break;
	}

	return S_OK;
}


HRESULT IfaceCalling CErrors_Filter::Get_Errors(const GUID *signal_id, const glucose::NError_Type type, glucose::TError_Markers *markers) {
	if (!mErrorCounter)
		return E_FAIL;

	return mErrorCounter->Get_Errors(*signal_id, type, *markers);
}
