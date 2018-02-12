#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"

namespace diffusion_v2_model {
	constexpr size_t param_count = 6;	//p, cg, c, dt, k, h
}

namespace steil_rebrin {
	constexpr size_t param_count = 4;	//g=1 => tau, alpha, beta, gamma
}


namespace newuoa {
	constexpr GUID id = { 0xfd4f3f19, 0xcd6b, 0x4598,{ 0x86, 0x32, 0x40, 0x84, 0x7a, 0xad, 0x9f, 0x5 } };	// {FD4F3F19-CD6B-4598-8632-40847AAD9F05}	
}

namespace metade {
	constexpr GUID id =	{ 0x1b21b62f, 0x7c6c, 0x4027,{ 0x89, 0xbc, 0x68, 0x7d, 0x8b, 0xd3, 0x2b, 0x3c } };		// {1B21B62F-7C6C-4027-89BC-687D8BD32B3C}

}

extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);