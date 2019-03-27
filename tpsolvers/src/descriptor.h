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

#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"


namespace nlopt {
	constexpr GUID newuoa_id = { 0xfd4f3f19, 0xcd6b, 0x4598,{ 0x86, 0x32, 0x40, 0x84, 0x7a, 0xad, 0x9f, 0x5 } };	// {FD4F3F19-CD6B-4598-8632-40847AAD9F05}
	constexpr GUID bobyqa_id = { 0x46559ef3, 0xa11c, 0x4387,{ 0x8f, 0x96, 0x47, 0x2d, 0x26, 0x71, 0xa, 0x1e } };	// {46559EF3-A11C-4387-8F96-472D26710A1E}
	constexpr GUID simplex_id = { 0x5f19b7de, 0x2a16, 0x4e2b, { 0x80, 0x9, 0x25, 0x1a, 0x2e, 0x9e, 0x1, 0xf0 } };
	constexpr GUID subplex_id = { 0x8342205, 0x2014, 0x4709, { 0x85, 0x76, 0x72, 0x95, 0x96, 0xc, 0x63, 0x78 } };
	constexpr GUID praxis_id = { 0x4b29deea, 0x72b8, 0x42e3, { 0xab, 0x56, 0x5, 0x39, 0xe4, 0xda, 0xab, 0xa0 } };

}

namespace pagmo {
	constexpr GUID pso_id = { 0x48b7e2f6, 0xa915, 0x4b63, { 0xb0, 0xf7, 0x18, 0x3e, 0xc0, 0x9b, 0x2, 0x5d } };
	constexpr GUID sade_id = { 0x90f4d682, 0xd8e, 0x4126, { 0x95, 0xc4, 0x9d, 0xb6, 0x19, 0x63, 0x3a, 0xa8 } };
	constexpr GUID de1220_id = { 0xb5ca8160, 0xf646, 0x4d53, { 0x8d, 0x66, 0xd9, 0xa8, 0xab, 0x25, 0x19, 0xbc } };
	constexpr GUID abc_id = { 0x1663854d, 0xed2e, 0x4493, { 0xa6, 0xe4, 0x46, 0xcc, 0x3f, 0xec, 0x98, 0x34 } };
	constexpr GUID cmaes_id = { 0x4e44d9f0, 0xd5d2, 0x430a, { 0x99, 0x8, 0x98, 0x13, 0xb0, 0x7c, 0x77, 0xc6 } };
	constexpr GUID xnes_id = { 0x100f6539, 0x63bd, 0x4a64, { 0x82, 0xe9, 0x86, 0x70, 0xb8, 0x0, 0xca, 0x11 } };
	constexpr GUID gpso_id = { 0x1f24727f, 0xe423, 0x4e2a, { 0xb8, 0xfe, 0xd3, 0x1, 0x64, 0xf7, 0x5a, 0x29 } };
	constexpr GUID ihs_id =  { 0xc3522bcd, 0x9ddc, 0x4655, { 0xaf, 0x7f, 0xe, 0x24, 0xdf, 0x66, 0x2d, 0xa0 } };
}




extern "C" HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);