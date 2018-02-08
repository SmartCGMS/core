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
	constexpr GUID id = { 0xfd4f3f19, 0xcd6b, 0x4598,{ 0x86, 0x32, 0x40, 0x84, 0x7a, 0xad, 0x9f, 0x5 } };// {FD4F3F19-CD6B-4598-8632-40847AAD9F05}	
}

extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);