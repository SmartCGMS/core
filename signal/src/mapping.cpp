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
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "mapping.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <iostream>
#include <chrono>

CMapping_Filter::CMapping_Filter(glucose::SFilter_Pipe_Reader inpipe, glucose::SFilter_Pipe_Writer outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }
{
	//
}


HRESULT IfaceCalling CMapping_Filter::Configure(glucose::IFilter_Configuration* configuration) {
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);
	mSource_Id = shared_configuration.Read_GUID(rsSignal_Source_Id);
	mDestination_Id = shared_configuration.Read_GUID(rsSignal_Destination_Id);

	return S_OK;
}

HRESULT IfaceCalling CMapping_Filter::Execute() {
	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
		if (!evt) break;

		if (evt.signal_id() == mSource_Id)
			evt.signal_id() = mDestination_Id;

		if (!mOutput.Send(evt))
			break;
	}

	return S_OK;

}