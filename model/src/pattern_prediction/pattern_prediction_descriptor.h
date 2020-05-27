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



namespace pattern_prediction {        

	const GUID filter_id = { 0xa730a576, 0xe84d, 0x4834, { 0x82, 0x6f, 0xfa, 0xee, 0x56, 0x4e, 0x6a, 0xbd } };  // {A730A576-E84D-4834-826F-FAEE564E6ABD}
    constexpr const GUID signal_Pattern_Prediction = { 0x4f9d0e51, 0x65e3, 0x4aaf, { 0xa3, 0x87, 0xd4, 0xd, 0xee, 0xe0, 0x72, 0x50 } }; 		// {4F9D0E51-65E3-4AAF-A387-D40DEEE07250}
   

    scgms::TSignal_Descriptor get_sig_desc();

    scgms::TFilter_Descriptor get_filter_desc();    //func to avoid static init fiasco as this is another unit than descriptor.cpp
}