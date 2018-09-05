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

namespace calculate
{
	constexpr GUID Calculate_Filter_GUID = { 0x14a25f4c, 0xe1b1, 0x85c4,{ 0x12, 0x74, 0x9a, 0x0d, 0x11, 0xe0, 0x98, 0x13 } }; //// {14A25F4C-E1B1-85C4-1274-9A0D11E09813}
}


extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter);
extern "C" HRESULT IfaceCalling do_create_signal(const GUID *signal_id, glucose::ITime_Segment *segment, glucose::ISignal **signal);
