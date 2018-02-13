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


extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
