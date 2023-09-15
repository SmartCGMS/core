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

#pragma once

#include "../../../common/rtl/guid.h"

namespace oref_model
{
	constexpr const GUID model_id = { 0x7ce8c304, 0x28ea, 0x4541, { 0x9d, 0x65, 0x95, 0x9e, 0x5b, 0x29, 0x25, 0x5b } }; // {7CE8C304-28EA-4541-9D65-959E5B29255B}

	constexpr const GUID oref0_basal_rate_signal_id = { 0xf077113f, 0x98b2, 0x4d8c, { 0xa0, 0xa2, 0x17, 0x51, 0x95, 0x91, 0x2, 0xc4 } };// {F077113F-98B2-4D8C-A0A2-1751959102C4}
	constexpr const GUID oref0_smb_signal_id = { 0x65089ea1, 0xf61, 0x4920, { 0xac, 0x9, 0x64, 0x7f, 0xa4, 0x91, 0xac, 0xaa } }; // {65089EA1-0F61-4920-AC09-647FA491ACAA}


	const size_t param_count = 3;

	const double lower_bound[param_count] = { 0.0, 0.0, 0.0 };
	const double default_parameters[param_count] = { 1.5, 0.0666, 1.3 };
	const double upper_bound[param_count] = { 4.0, 2.0, 5.0 };

	struct TParameters {
		union {
			struct {
				double isf;
				double cr;
				double ibr;
			};
			double vector[param_count];
		};
	};
}
