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
#include "../../../common/rtl/ModelsLib.h"

namespace bergman_model {
	constexpr GUID filter_id = { 0x10659c91, 0x1ffb, 0x403c, { 0xbd, 0xf6, 0x35, 0xb2, 0x56, 0xb3, 0x8, 0x1 } };	// {10659C91-1FFB-403C-BDF6-35B256B30801}
	constexpr GUID model_id = { 0x8114b2a6, 0xb4b2, 0x4c8d, { 0xa0, 0x29, 0x62, 0x5c, 0xbd, 0xb6, 0x82, 0xef } };


	constexpr size_t model_param_count = 16;

	struct TParameters {
		union {
			struct {
				double p1, p2, p3, p4;
				double Vi;
				double BodyWeight;
				double VgDist;
				double d1rate, d2rate;
				double irate;
				double Gb, Ib;
				double G0;
				double p, cg, c;
			};
			double vector[model_param_count];
		};
	};

	constexpr TParameters lower_bounds = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.5, -5.0 };
	constexpr TParameters default_parameters = { 0.028735, 0.028344, 5.035e-05, 0.3, 12.0, 70.0, 0.22, 0.05, 0.05, 0.04, 95.0, 9.2, 100.0, 0.929, -0.037, 1.308 };
	constexpr TParameters upper_bounds = { 1.0, 1.0, 1.0, 1.0, 20.0, 10.0, 1.0, 1.0, 1.0, 1.0, 200.0, 20.0, 200.0, 2.0, 0.0, 5.0};
}

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter *output, glucose::IFilter **filter);
extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);