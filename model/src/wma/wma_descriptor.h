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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include "../../../../common/iface/UIIface.h"
#include "../../../../common/rtl/hresult.h"
#include "../../../../common/rtl/ModelsLib.h"
#include "../../../../common/rtl/guid.h"



namespace wma {        

	constexpr GUID model_id = { 0x2b516dd9, 0xe1ba, 0x4df6, { 0xb8, 0xe6, 0x9c, 0xec, 0xd4, 0x2e, 0xf8, 0xda } };  // {2B516DD9-E1BA-4DF6-B8E6-9CECD42EF8DA}
    constexpr const GUID signal_id = { 0x4798e566, 0xa225, 0x4253, { 0xa6, 0x86, 0x70, 0xb3, 0xd1, 0xaf, 0x66, 0x7a } };  // {4798E566-A225-4253-A686-70B3D1AF667A}

	constexpr size_t coeff_count = 12;
	constexpr size_t model_param_count = coeff_count + 1;
	
	struct TParameters {
		union {
			struct {
				double dt;
				double coeff[coeff_count];
					//from -12*5 minutes up to current time
			};
			double vector[model_param_count];
		};
	};

	extern const double default_parameters[model_param_count];

   

    scgms::TSignal_Descriptor get_sig_desc();

    scgms::TModel_Descriptor get_model_desc();    //func to avoid static init fiasco as this is another unit than descriptor.cpp
}
