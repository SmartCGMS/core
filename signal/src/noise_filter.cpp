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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "noise_filter.h"

#include <scgms/rtl/FilterLib.h>
#include <scgms/lang/dstrings.h>

#include <algorithm>
#include <numeric>

CWhite_Noise_Generator_Filter::CWhite_Noise_Generator_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

HRESULT IfaceCalling CWhite_Noise_Generator_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mSignal_Id = configuration.Read_GUID(rsSignal_Id);

	double noise_max = configuration.Read_Double(L"Noise_Max", 1.0);
	mRandom_Distribution = std::uniform_real_distribution<double>(-noise_max, noise_max);

	return S_OK;
}

HRESULT IfaceCalling CWhite_Noise_Generator_Filter::Do_Execute(scgms::UDevice_Event event)
{
	if (event.signal_id() == mSignal_Id && event.is_level_event()) {
		event.level() += mRandom_Distribution(mRandom_Engine);
	}

	return mOutput.Send(event);
}
