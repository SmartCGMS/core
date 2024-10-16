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

namespace mt_metade {	//mersenne twister initialized with linear random generator
	constexpr GUID id =	{ 0x1b21b62f, 0x7c6c, 0x4027,{ 0x89, 0xbc, 0x68, 0x7d, 0x8b, 0xd3, 0x2b, 0x3c } };	// {1B21B62F-7C6C-4027-89BC-687D8BD32B3C}
}

namespace halton_metade {
	constexpr GUID id = { 0x1274b08, 0xf721, 0x42bc, { 0xa5, 0x62, 0x5, 0x56, 0x71, 0x4c, 0x56, 0x85 } };
}

namespace rnd_metade {		//std::random_device, should be cryprographicallly secure depending on the implementation
	constexpr GUID id = { 0x2332f9a7, 0x39a2, 0x4fd6, { 0x90, 0xd5, 0x90, 0xb8, 0x85, 0x20, 0x18, 0x69 } };
}

namespace xss_metade {
	constexpr GUID id= { 0x3c970189, 0x2d90, 0x43aa, { 0xb9, 0x9b, 0xc7, 0x1c, 0x29, 0xab, 0x9f, 0x47 } }; // {3C970189-2D90-43AA-B99B-C71C29AB9F47}
}

namespace pso {
	constexpr GUID id = { 0x93fff43a, 0x50e8, 0x4c7b, { 0x82, 0xf0, 0xf2, 0x90, 0xdf, 0xf2, 0x8, 0x9c } };	// {93FFF43A-50E8-4C7B-82F0-F290DFF2089C}
	constexpr GUID pr_id = { 0xb93fea5a, 0x11fc, 0x405b, { 0xa9, 0x64, 0x5c, 0x93, 0xba, 0x17, 0x42, 0xc5 } }; // {B93FEA5A-11FC-405B-A964-5C93BA1742C5}
}

namespace sequential_brute_force_scan {
	constexpr GUID id = { 0x33d92b0, 0xb49c, 0x45d1, { 0x95, 0x7f, 0x57, 0x68, 0x2d, 0x56, 0xab, 0xd2 } };  //{033D92B0-B49C-45D1-957F-57682D56ABD2}
}

namespace sequential_convex_scan {
	constexpr GUID id = { 0xfa42286b, 0x928c, 0x47bd, { 0xaa, 0xf3, 0x2f, 0xe4, 0x73, 0x77, 0x46, 0x6d } };	//{FA42286B-928C-47BD-AAF3-2FE47377466D}
}

namespace mutation {
	constexpr GUID id = { 0x8187dbdc, 0x4394, 0x4cb0, { 0xae, 0xba, 0xa6, 0x86, 0x53, 0xc7, 0x7b, 0x98 } };	// {8187DBDC-4394-4CB0-AEBA-A68653C77B98}
}

namespace rumoropt {
	constexpr GUID id = { 0x6bad021f, 0x6f68, 0x4246, { 0xa4, 0xe6, 0x7b, 0x19, 0x50, 0xca, 0x71, 0xcb } };	// {6BAD021F-6F68-4246-A4E6-7B1950CA71CB}
}
