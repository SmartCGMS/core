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


namespace diffusion_v2_model {
	constexpr GUID signal_Diffusion_v2_Blood = { 0xd96a559b, 0xe247, 0x41e0,{ 0xbd, 0x8e, 0x78, 0x8d, 0x20, 0xdb, 0x9a, 0x70 } }; // {D96A559B-E247-41E0-BD8E-788D20DB9A70}
	constexpr GUID signal_Diffusion_v2_Ist = { 0x870ddbd6, 0xdaf1, 0x4877,{ 0xa8, 0x9e, 0x5e, 0x7b, 0x2, 0x8d, 0xa6, 0xc7 } };  // {870DDBD6-DAF1-4877-A89E-5E7B028DA6C7}


	const double default_parameters[param_count] = { 1.091213, -0.015811, -0.174114, 0.0233101854166667, -2.6E-6, 0.0185995368055556 };

	struct TParameters {
		union {
			struct {
				double p, cg, c, dt, k, h;
			};
			double vector[param_count];
		};
	};
}


namespace steil_rebrin {
	constexpr GUID signal_Steil_Rebrin_Blood = { 0x287b9bb8, 0xb73b, 0x4485,{ 0xbe, 0x20, 0x2c, 0x8c, 0x40, 0x98, 0x3b, 0x16 } }; // {287B9BB8-B73B-4485-BE20-2C8C40983B16}

	//DelFavero 2014 flaw of the Steil-Rebrin model
	const double default_parameters[param_count] = { 0.00576459, 1.0 / 1.02164072, 0.0, 0.0 };

	struct TParameters {
		union {
			struct {
				double tau, alpha, beta, gamma;	//and g is considered == 1.0
			};
			double vector[param_count];
		};
	};
}

namespace steil_rebrin_diffusion_prediction {
	constexpr GUID signal_Steil_Rebrin_Diffusion_Prediction = { 0xf997edf4, 0x357c, 0x4cb2, { 0x8d, 0x7c, 0xf6, 0x81, 0xa3, 0x76, 0xe1, 0x7c } };

	//const double default_parameters[param_count] = { 1.091213, -0.015811, -0.174114, 0.0233101854166667, 1.02164072, 1.0 / 0.00576459  };
	const double default_parameters[param_count] = { 0.00116362, -7.20151e-11, 0.0794574, 0.010417, 850.437, 0.00630974 };	//15 minutes prediction

	struct TParameters {
		union {
			struct {
				double p, cg, c, dt, inv_g, tau;
			};
			double vector[param_count];
		};
	};
}


namespace diffusion_prediction {
	constexpr GUID signal_Diffusion_Prediction = { 0x43fcd03d, 0xb8bc, 0x497d, { 0x9e, 0xab, 0x3d, 0x1e, 0xeb, 0x3e, 0xbb, 0x5c } };

	const double default_parameters[param_count] = { 1.091213, -0.015811, -0.174114, 0.0233101854166667, 1.065754, -0.004336, 0.167069, 0.0247337965277778 - 0.0233101854166667 };

	struct TParameters {
		union {
			struct {
				double p, cg, c, dt;
			} retrospective, predictive;
			double vector[param_count];
		};		
	};
}

namespace constant_model {

	constexpr GUID signal_Constant = { 0xfc616dd, 0x7bf, 0x49ea, { 0xbf, 0x2c, 0xfc, 0xa1, 0x39, 0x42, 0x9f, 0x5 } };


	const size_t param_count = 1;
	const double default_parameters[param_count] = { 6.66 };

	struct TParameters {
		union {
			struct {
				double c;
			};
			double vector[param_count];
		};
	};
}

namespace bergman_model {
	
	constexpr GUID model_id = { 0x8114b2a6, 0xb4b2, 0x4c8d, { 0xa0, 0x29, 0x62, 0x5c, 0xbd, 0xb6, 0x82, 0xef } };

	constexpr GUID signal_Bergman_BG = { 0xe1cd07ef, 0xb079, 0x4911, { 0xb7, 0x9b, 0xd2, 0x3, 0x48, 0x61, 0x1, 0xc8 } };				// {E1CD07EF-B079-4911-B79B-D203486101C8}
	constexpr GUID signal_Bergman_IG = { 0x582cfe11, 0x68b9, 0x4e5e, { 0xb5, 0xb, 0x19, 0x78, 0xc0, 0x77, 0xa8, 0x4e } };				// {582CFE11-68B9-4E5E-B50B-1978C077A84E}
	constexpr GUID signal_Bergman_IOB = { 0x9cddc004, 0x12ca, 0x4380, { 0x96, 0xff, 0x83, 0x19, 0xa8, 0xce, 0x79, 0xa8 } };				// {9CDDC004-12CA-4380-96FF-8319A8CE79A8}
	constexpr GUID signal_Bergman_COB = { 0x3bd439f0, 0x8b5a, 0x4004, { 0x9b, 0x70, 0x24, 0xe5, 0xe2, 0x52, 0x3b, 0x7a } };				// {3BD439F0-8B5A-4004-9B70-24E5E2523B7A}
	constexpr GUID signal_Bergman_Basal_Insulin = { 0xfc556839, 0xd6c0, 0x4646, { 0xa3, 0x46, 0x8, 0x21, 0xd2, 0x5f, 0x7e, 0x29 } };	// {FC556839-D6C0-4646-A346-0821D25F7E29}
	constexpr GUID signal_Bergman_Insulin_Activity = { 0x755cfd08, 0x2b12, 0x43b6, { 0xa4, 0x55, 0x58, 0x6, 0x15, 0x68, 0x44, 0x6e } };	// {755CFD08-2B12-43B6-A455-58061568446E}



	constexpr size_t model_param_count = 23;

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
				double G0, X0, I0, D10, D20, Isc0, Gsc0;
				double BasalRate0;
				double p, cg, c;
			};
			double vector[model_param_count];
		};
	};

	constexpr bergman_model::TParameters lower_bounds = { 0.0, 0.0, 0.0, 0.0, 8.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.5, -5.0 };
	constexpr bergman_model::TParameters default_parameters = { 0.028735, 0.028344, 5.035e-05, 0.3, 12.0, 70.0, 0.22, 0.05, 0.05, 0.04, 95.0, 9.2, 100.0, 0, 0, 0, 0, 0, 95.0, 0, 0.929, -0.037, 1.308 };
	constexpr bergman_model::TParameters upper_bounds = { 0.1, 0.1, 0.05, 1.0, 18.0, 100.0, 1.0, 1.0, 1.0, 1.0, 200.0, 20.0, 300.0, 100.0, 200.0, 150.0, 150.0, 50.0, 300.0, 5.0, 2.0, 0.0, 5.0 };
}



extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, glucose::IModel_Parameter_Vector *parameters, glucose::IFilter *output, glucose::IDiscrete_Model **model);