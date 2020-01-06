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

namespace iob {
	constexpr size_t param_count = 2;
	const double default_parameters[param_count] = { 75.0*scgms::One_Minute, 180.0*scgms::One_Minute };

	struct TParameters {
		union {
			struct {
				double peak;
				double dia;
			};
			double vector[param_count];
		};
	};
}

namespace cob {
	constexpr size_t param_count = 2;
	const double default_parameters[param_count] = { 75.0*scgms::One_Minute, 180.0*scgms::One_Minute };

	struct TParameters {
		union {
			struct {
				double peak;
				double dia;	// TODO: rework parameters to fit COB calc!!! for now we wrongly retain IOB params
			};
			double vector[param_count];
		};
	};
}

namespace betapid_insulin_regulation {
	constexpr const GUID betapid_signal_id = { 0xd7eeda17, 0x781b, 0x438d, { 0xb9, 0x9b, 0x12, 0x81, 0xb9, 0x71, 0x33, 0x6e } };						// {D7EEDA17-781B-438D-B99B-1281B971336E}
	constexpr const GUID betapid2_signal_id = { 0x4b574705, 0x3832, 0x478d, { 0xa3, 0xae, 0xb6, 0xd8, 0x27, 0x74, 0x1f, 0xd } };						// {4B574705-3832-478D-A3AE-B6D827741F0D}

	const size_t param_count = 6;
	const double default_parameters[param_count] = { 2.0, 0.028, 0.24, 0.36, 0.0667, 1.3 };

	struct TParameters {
		union {
			struct {
				double kp, ki, kd, isf, csr, bin;
			};
			double vector[param_count];
		};
	};
}

namespace betapid3_insulin_regulation
{
	constexpr const GUID betapid3_signal_id = { 0xda20eb00, 0xe8bc, 0x42a7, { 0x8e, 0x11, 0xbf, 0xc9, 0xa7, 0xf7, 0x79, 0x5 } };						// {DA20EB00-E8BC-42A7-8E11-BFC9A7F77905}

	const size_t param_count = 5;
	const double default_parameters[param_count] = { 2.0, 0.028, 0.24, 0.95, 1.3 };

	struct TParameters {
		union {
			struct {
				double k, ki, kd, kidecay, bin;
			};
			double vector[param_count];
		};
	};
}

namespace lgs_basal_insulin
{
	constexpr const GUID lgs_basal_insulin_signal_id = { 0x6957caf0, 0x797, 0x442b, { 0xa0, 0x0, 0xe3, 0x94, 0xbb, 0x1e, 0x7d, 0x3d } };	// {6957CAF0-0797-442B-A000-E394BB1E7D3D}

	const size_t param_count = 3;

	const double lower_bound[param_count] = { 0.0, 0.0, 0.0 };
	const double default_parameters[param_count] = { 1.3, 4.0, 30.0 * (1.0 / (24.0*60.0)) };
	const double upper_bound[param_count] = { 3.0, 8.0, 120.0 * (1.0 / (24.0*60.0)) };

	struct TParameters {
		union {
			struct {
				double bin;
				double lower_threshold;
				double suspend_duration;
			};
			double vector[param_count];
		};
	};
}

extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_signal(const GUID *calc_id, scgms::ITime_Segment *segment, scgms::ISignal **signal);
