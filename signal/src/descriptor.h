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

namespace calculate
{
	constexpr GUID Calculate_Filter_GUID = { 0x14a25f4c, 0xe1b1, 0x85c4,{ 0x12, 0x74, 0x9a, 0x0d, 0x11, 0xe0, 0x98, 0x13 } }; //// {14A25F4C-E1B1-85C4-1274-9A0D11E09813}
}

namespace signal_generator {
	constexpr GUID filter_id = { 0x9eeb3451, 0x2a9d, 0x49c1, { 0xba, 0x37, 0x2e, 0xc0, 0xb0, 0xe, 0x5e, 0x6d } };  // {9EEB3451-2A9D-49C1-BA37-2EC0B00E5E6D}
}

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor **begin, scgms::TSignal_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter);
extern "C" HRESULT IfaceCalling do_create_signal(const GUID *signal_id, scgms::ITime_Segment *segment, const GUID * approx_id, scgms::ISignal **signal);
