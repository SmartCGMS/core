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

extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
