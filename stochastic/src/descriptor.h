/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"


namespace newuoa {
	constexpr GUID id = { 0xfd4f3f19, 0xcd6b, 0x4598,{ 0x86, 0x32, 0x40, 0x84, 0x7a, 0xad, 0x9f, 0x5 } };	// {FD4F3F19-CD6B-4598-8632-40847AAD9F05}	
}

namespace metade {
	constexpr GUID id =	{ 0x1b21b62f, 0x7c6c, 0x4027,{ 0x89, 0xbc, 0x68, 0x7d, 0x8b, 0xd3, 0x2b, 0x3c } };	// {1B21B62F-7C6C-4027-89BC-687D8BD32B3C}

}

namespace bobyqa {
	constexpr GUID id = { 0x46559ef3, 0xa11c, 0x4387,{ 0x8f, 0x96, 0x47, 0x2d, 0x26, 0x71, 0xa, 0x1e } };	// {46559EF3-A11C-4387-8F96-472D26710A1E}
}


extern "C" HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);