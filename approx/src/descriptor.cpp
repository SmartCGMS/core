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

#include "line.h"
#include "Akima.h"
#include "avgexp/AvgExpApprox.h"
#include "avgexp/AvgElementaryFunctions.h"

#include <scgms/iface/UIIface.h>
#include <scgms/iface/ApproxIface.h>
#include <scgms/lang/dstrings.h>
#include <scgms/rtl/manufactory.h>
#include <scgms/rtl/hresult.h>

#include <vector>

namespace line {	
	const scgms::TApprox_Descriptor LineApprox_Descriptor = {
		scgms::apxLine,
		dsLine_Approx
	};
}

namespace akima {
	const scgms::TApprox_Descriptor Akima_Descriptor = {
		scgms::apxAkima,	
		dsAkima		
	};
}

namespace avgexp {
	const scgms::TApprox_Descriptor AvgExp_Descriptor = {
		scgms::apxAvgExpAppx,
		dsAvgExp
	};
}


const std::array<scgms::TApprox_Descriptor, 3> approx_descriptions = { { line::LineApprox_Descriptor, 
																		  akima::Akima_Descriptor,
																		 avgexp::AvgExp_Descriptor	} };

DLL_EXPORT HRESULT IfaceCalling do_get_approximator_descriptors(scgms::TApprox_Descriptor **begin, scgms::TApprox_Descriptor **end) {
	*begin = const_cast<scgms::TApprox_Descriptor*>(approx_descriptions.data());
	*end = *begin + approx_descriptions.size();
	return S_OK;
}

DLL_EXPORT HRESULT IfaceCalling do_create_approximator(const GUID *approx_id, scgms::ISignal *signal, scgms::IApproximator **approx) {

	if (approx_id == nullptr)	//if no id is given, let's use the default approximator
		return Manufacture_Object<CAkima>(approx, scgms::WSignal{ signal });

	if (*approx_id == line::LineApprox_Descriptor.id)
		return Manufacture_Object<CLine_Approximator>(approx, scgms::WSignal{ signal });
	else if (*approx_id == akima::Akima_Descriptor.id)
		return Manufacture_Object<CAkima>(approx, scgms::WSignal{ signal });
	else if (*approx_id == avgexp::AvgExp_Descriptor.id)
		return Manufacture_Object<CAvgExpApprox>(approx, scgms::WSignal{ signal }, AvgExpElementary);

	return E_NOTIMPL;
}
