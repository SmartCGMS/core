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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <scgms/iface/UIIface.h>
#include <scgms/rtl/hresult.h>


namespace nlopt {
	constexpr GUID newuoa_id = { 0xfd4f3f19, 0xcd6b, 0x4598,{ 0x86, 0x32, 0x40, 0x84, 0x7a, 0xad, 0x9f, 0x5 } };	// {FD4F3F19-CD6B-4598-8632-40847AAD9F05}
	constexpr GUID bobyqa_id = { 0x46559ef3, 0xa11c, 0x4387,{ 0x8f, 0x96, 0x47, 0x2d, 0x26, 0x71, 0xa, 0x1e } };	// {46559EF3-A11C-4387-8F96-472D26710A1E}
	constexpr GUID simplex_id = { 0x5f19b7de, 0x2a16, 0x4e2b, { 0x80, 0x9, 0x25, 0x1a, 0x2e, 0x9e, 0x1, 0xf0 } };
	constexpr GUID subplex_id = { 0x8342205, 0x2014, 0x4709, { 0x85, 0x76, 0x72, 0x95, 0x96, 0xc, 0x63, 0x78 } };
	constexpr GUID praxis_id = { 0x4b29deea, 0x72b8, 0x42e3, { 0xab, 0x56, 0x5, 0x39, 0xe4, 0xda, 0xab, 0xa0 } };
	constexpr GUID cobyla_id = { 0xf6cf1399, 0x994e, 0x4f0c, { 0x89, 0xa5, 0xd3, 0x2, 0xd8, 0xe7, 0xff, 0x3 } };	// {F6CF1399-994E-4F0C-89A5-D302D8E7FF03}

}
