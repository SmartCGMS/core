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

extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
