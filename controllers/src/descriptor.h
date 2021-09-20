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
				double kp, ki, kd, isf, csr, basal_insulin_rate;
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
				double k, ki, kd, kidecay, basal_insulin_rate;
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
				double basal_insulin_rate;
				double suspend_threshold;
				double suspend_duration;
			};
			double vector[param_count];
		};
	};
}

namespace physical_activity_detection
{
	constexpr const GUID signal_id = { 0x5596068a, 0x4acf, 0x4e3f, { 0x9a, 0x98, 0xda, 0x6b, 0x9f, 0xe5, 0xe1, 0x9f } };	// {5596068A-4ACF-4E3F-9A98-DA6B9FE5E19F}

	const size_t param_count = 3;

	const double lower_bound[param_count] = { 30.0, 2.0, 15.0 };
	const double default_parameters[param_count] = { 65.0, 10.0, 20.0 };
	const double upper_bound[param_count] = { 100.0, 15.0, 30.0 };

	struct TParameters {
		union {
			struct {
				double heartrate_resting;
				double eda_threshold, eda_max;
			};
			double vector[param_count];
		};
	};
}

namespace gege
{
	constexpr const GUID model_id = { 0x7be59b46, 0xb5db, 0x412c, { 0x96, 0x16, 0x30, 0xe7, 0x88, 0x32, 0xf2, 0xf0 } };		// {7BE59B46-B5DB-412C-9616-30E78832F2F0}

	constexpr const GUID ibr_id = { 0xac22fa49, 0x154f, 0x435e, { 0x9e, 0xdc, 0x36, 0xb, 0x87, 0x69, 0xec, 0x8b } };		// {AC22FA49-154F-435E-9EDC-360B8769EC8B}
	constexpr const GUID bolus_id = { 0x6741202c, 0x247d, 0x4430, { 0xaf, 0x45, 0x17, 0xf2, 0xc0, 0xcf, 0x73, 0xa9 } };		// {6741202C-247D-4430-AF45-17F2C0CF73A9}

	constexpr size_t param_count = 60;

	struct TParameters {
		union {
			struct {
				double codons[param_count];
				// may be split to some logical units in the future (e.g.; codons coding the grammar and parameters of every node)
			};
			double vector[param_count];
		};
	};

	const TParameters lower_bounds = {
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
	};
	const TParameters default_parameters = {
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
	};
	const TParameters upper_bounds = {
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
	};

}

namespace flr_ge
{
	constexpr const GUID model_id = { 0xbf37b3fc, 0xb794, 0x46fc, { 0xae, 0xd0, 0xb, 0xf1, 0xea, 0x83, 0xdd, 0x4b } };		// {BF37B3FC-B794-46FC-AED0-0BF1EA83DD4B}
	constexpr const GUID ibr_id = { 0xa2f6b465, 0x9f78, 0x421f, { 0x83, 0x98, 0x96, 0x7d, 0xc9, 0xd7, 0xc, 0xd8 } };		// {A2F6B465-9F78-421F-8398-967DC9D70CD8}
	constexpr const GUID bolus_id = { 0xde4ca4f0, 0xddcf, 0x4974, { 0x8d, 0xab, 0x1b, 0x9e, 0xd, 0xb9, 0x45, 0xef } };		// {DE4CA4F0-DDCF-4974-8DAB-1B9E0DB945EF}

	constexpr size_t rules_count = 20;
	constexpr size_t rule_size = 4;
	constexpr size_t constants_count = 20; // should be more

	constexpr size_t param_count = rules_count * rule_size + constants_count;

	struct TParameters {
		union {
			struct {
				double rules[rules_count * rule_size];
				double constants[constants_count];
			};
			double vector[param_count];
		};
	};

	const TParameters lower_bounds = {
		// rules
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		// constants
		-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,
		-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,-100.0,
	};
	const TParameters default_parameters = {
		// rules
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		// constants
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
	};
	const TParameters upper_bounds = {
		// rules
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		// constants
		100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0,
		100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0,
	};

}

extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end);
extern "C" HRESULT IfaceCalling do_create_signal(const GUID *calc_id, scgms::ITime_Segment *segment, const GUID * approx_id, scgms::ISignal **signal);
extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID * model_id, scgms::IModel_Parameter_Vector * parameters, scgms::IFilter * output, scgms::IDiscrete_Model * *model);
