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

#include <scgms/iface/UIIface.h>
#include <scgms/rtl/hresult.h>
#include <scgms/rtl/ModelsLib.h>


namespace basal_2_bolus {

	constexpr GUID filter_id = { 0xebf3a9e5, 0x5b00, 0x43ef, { 0xb6, 0x64, 0xde, 0x30, 0xcc, 0xd1, 0xa3, 0x7d } }; // {EBF3A9E5-5B00-43EF-B664-DE30CCD1A37D}

	constexpr size_t model_param_count = 2;
	const double default_parameters[model_param_count] = { 0.05, 5.0 * scgms::One_Minute };	//https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4455475/

	struct TParameters {
		union {
			struct {
				double minimum_amount;		//e.g.; the insulin pump motor delivers 0.05 IU/hour
				double period;				//every 5 minutes
			};
			double vector[model_param_count];
		};
	};

	extern const scgms::TFilter_Descriptor filter_desc;
	extern const scgms::TModel_Descriptor model_desc;
}

