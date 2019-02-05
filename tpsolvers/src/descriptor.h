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


namespace newuoa {
	constexpr GUID id = { 0xfd4f3f19, 0xcd6b, 0x4598,{ 0x86, 0x32, 0x40, 0x84, 0x7a, 0xad, 0x9f, 0x5 } };	// {FD4F3F19-CD6B-4598-8632-40847AAD9F05}
}

namespace bobyqa {
	constexpr GUID id = { 0x46559ef3, 0xa11c, 0x4387,{ 0x8f, 0x96, 0x47, 0x2d, 0x26, 0x71, 0xa, 0x1e } };	// {46559EF3-A11C-4387-8F96-472D26710A1E}
}

namespace pso {
	constexpr GUID id = { 0x48b7e2f6, 0xa915, 0x4b63, { 0xb0, 0xf7, 0x18, 0x3e, 0xc0, 0x9b, 0x2, 0x5d } };
}

namespace sade {
	constexpr GUID id = { 0x90f4d682, 0xd8e, 0x4126, { 0x95, 0xc4, 0x9d, 0xb6, 0x19, 0x63, 0x3a, 0xa8 } };
}

namespace de1220 {
	constexpr GUID id = { 0xb5ca8160, 0xf646, 0x4d53, { 0x8d, 0x66, 0xd9, 0xa8, 0xab, 0x25, 0x19, 0xbc } };
}

namespace abc {
	constexpr GUID id = { 0x1663854d, 0xed2e, 0x4493, { 0xa6, 0xe4, 0x46, 0xcc, 0x3f, 0xec, 0x98, 0x34 } };
}

namespace cmaes {
	constexpr GUID id = { 0x4e44d9f0, 0xd5d2, 0x430a, { 0x99, 0x8, 0x98, 0x13, 0xb0, 0x7c, 0x77, 0xc6 } };
}

namespace xnes {
	constexpr GUID id = { 0x100f6539, 0x63bd, 0x4a64, { 0x82, 0xe9, 0x86, 0x70, 0xb8, 0x0, 0xca, 0x11 } };
}




extern "C" HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);
