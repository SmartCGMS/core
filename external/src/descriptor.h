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
#include "../../../common/rtl/ModelsLib.h"

namespace dmms_model
{
	constexpr GUID proc_model_id = { 0xafac0ea1, 0xbd17, 0x4f68, { 0x9b, 0x2d, 0xec, 0x7b, 0xbd, 0x61, 0x9b, 0x69 } };			// {AFAC0EA1-BD17-4F68-9B2D-EC7BBD619B69}
	constexpr GUID lib_model_id = { 0xe0971cfa, 0xcc8c, 0x42ec, { 0xba, 0x3, 0xda, 0xf7, 0x49, 0xb7, 0xa9, 0x1d } };			// {E0971CFA-CC8C-42EC-BA03-DAF749B7A91D}


	constexpr GUID signal_DMMS_BG = { 0x80716eea, 0x2b17, 0x42b6, { 0x9c, 0xc4, 0x94, 0x54, 0xc0, 0x25, 0x78, 0x94 } };				// {80716EEA-2B17-42B6-9CC4-9454C0257894}
	constexpr GUID signal_DMMS_IG = { 0xa8a279db, 0x7615, 0x40d9, { 0xab, 0xeb, 0xbc, 0x62, 0xc5, 0x59, 0x10, 0x2 } };					// {A8A279DB-7615-40D9-ABEB-BC62C5591002}
	constexpr GUID signal_DMMS_Delivered_Insulin_Basal = { 0x70ae658f, 0x631d, 0x4a4b, { 0xb2, 0xb6, 0x3f, 0x3e, 0xe7, 0xd2, 0xf1, 0xdb } };		// {70AE658F-631D-4A4B-B2B6-3F3EE7D2F1DB}

	constexpr size_t model_param_count = 2;

	struct TParameters {
		union {
			struct {
				double meal_announce_offset;
				double excercise_announce_offset;
			};
			double vector[model_param_count];
		};
	};

	constexpr TParameters lower_bounds = {
		-30 * (1.0 / (24.0*60.0)),	// meal_announce_offset, -30 minutes
		-30 * (1.0 / (24.0*60.0))	// excercise_announce_offset, -30 minutes
	};
	constexpr TParameters default_parameters = {
		-10 * (1.0 / (24.0*60.0)),	// meal_announce_offset, 10 minutes
		-10 * (1.0 / (24.0*60.0)),	// excercise_announce_offset, 10 minutes
	};
	constexpr TParameters upper_bounds = {
		+30 * (1.0 / (24.0*60.0)),	// meal_announce_offset, 30 minutes
		+30 * (1.0 / (24.0*60.0)),	// excercise_announce_offset, 30 minutes
	};
}

namespace network_model
{
	constexpr GUID model_id = { 0xf2858227, 0x3c8d, 0x418e, { 0x9d, 0xc4, 0x5a, 0x3, 0x7, 0x13, 0xc9, 0xb2 } }; // {F2858227-3C8D-418E-9DC4-5A030713C9B2}
	constexpr size_t model_param_count = 0;

}

extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model);
extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor **begin, scgms::TSignal_Descriptor **end);
