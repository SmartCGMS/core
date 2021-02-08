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

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"



namespace mt_metade {	//mersenne twister initialized with linear random generator
	constexpr GUID id =	{ 0x1b21b62f, 0x7c6c, 0x4027,{ 0x89, 0xbc, 0x68, 0x7d, 0x8b, 0xd3, 0x2b, 0x3c } };	// {1B21B62F-7C6C-4027-89BC-687D8BD32B3C}
}

namespace halton_metade {
	constexpr GUID id = { 0x1274b08, 0xf721, 0x42bc, { 0xa5, 0x62, 0x5, 0x56, 0x71, 0x4c, 0x56, 0x85 } };
}

namespace rnd_metade {		//std::random_device, should be cryprographicallly secure depending on the implementation
	constexpr GUID id = { 0x2332f9a7, 0x39a2, 0x4fd6, { 0x90, 0xd5, 0x90, 0xb8, 0x85, 0x20, 0x18, 0x69 } };
}


namespace mt_unconstrainedmetade {
	constexpr GUID id = { 0x7dbf68f2, 0x8fe2, 0x47b1, { 0xb1, 0xc1, 0x97, 0x45, 0xea, 0x8a, 0x85, 0xd3 } }; // {7DBF68F2-8FE2-47B1-B1C1-9745EA8A85D3}
}


namespace pathfinder {
	constexpr GUID id_fast = { 0x787223e7, 0x6363, 0x41d0, { 0xb1, 0x3d, 0x93, 0xb5, 0xd4, 0xd9, 0x4b, 0xbd } };
    constexpr GUID id_spiral = { 0xbd3baa39, 0xc447, 0x436f, { 0xbf, 0x6c, 0xdf, 0x21, 0x38, 0xd5, 0x31, 0xd4 } };
	constexpr GUID id_landscape = { 0xc919978d, 0x1b6f, 0x4b34, { 0xb3, 0xe5, 0xf8, 0x38, 0xaf, 0xe, 0x8e, 0xfb } };
}

namespace sequential_brute_force_scan {		
	constexpr GUID id = { 0x33d92b0, 0xb49c, 0x45d1, { 0x95, 0x7f, 0x57, 0x68, 0x2d, 0x56, 0xab, 0xd2 } };  // {033D92B0-B49C-45D1-957F-57682D56ABD2}
}

extern "C" HRESULT IfaceCalling do_get_solver_descriptors(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end);
