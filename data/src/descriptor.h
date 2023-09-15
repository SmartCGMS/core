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

namespace db_reader {
	constexpr GUID filter_id = { 0x1f8ffb51, 0x5f8c, 0x416b, { 0x9b, 0x80, 0x93, 0x66, 0xb0, 0x81, 0x2f, 0xff } }; //// {1F8FFB51-5F8C-416B-9B80-9366B0812FFF}
}

namespace db_writer {
	constexpr GUID filter_id = { 0xb93222aa, 0xbec3, 0x4b0f, { 0x93, 0x8d, 0x4f, 0x44, 0x62, 0x31, 0x48, 0x3c } }; //// {B93222AA-BEC3-4B0F-938D-4F446231483C}
}

namespace db_reader_legacy {
	constexpr GUID filter_id = { 0xc0e942b9, 0x3928, 0x4b81, { 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0x42 } }; //// {C0E942B9-3928-4B81-9B43-A34766820042}
}

namespace db_writer_legacy {
	constexpr GUID filter_id = { 0x9612e8c4, 0xb841, 0x12f7,{ 0x8a, 0x1c, 0x76, 0xc3, 0x49, 0xa1, 0x62, 0x08 } }; //// {9612E8C4-B841-12F7-8A1C-76C349A16208}
}

namespace sincos_generator {
	constexpr GUID filter_id = { 0xcf42445d, 0x9526, 0x4414, { 0xa1, 0x3d, 0x2f, 0x54, 0xb9, 0x59, 0x27, 0x98 } }; //// {CF42445D-9526-4414-A13D-2F54B9592798}
}
