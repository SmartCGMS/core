#pragma once

#include "descriptor.h"
#include "line.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace line
{
	constexpr size_t param_count = 0;

	const glucose::TApprox_Descriptor LineApprox_Descriptor = {
		{ 0xb89204aa, 0x5842, 0xa8f1, { 0x4c, 0xa1, 0x43, 0x12, 0x5f, 0x4e, 0xb2, 0xa7 } },	//// {B89204AA-5842-A8F1-4CA1-43125F4EB2A7}
		dsLine_Approx,
		param_count,
		nullptr,
		nullptr
	};
}

static const std::vector<glucose::TApprox_Descriptor> approx_descriptions = { line::LineApprox_Descriptor };

extern "C" HRESULT IfaceCalling do_get_approximator_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end) {
	*begin = const_cast<glucose::TApprox_Descriptor*>(approx_descriptions.data());
	*end = *begin + approx_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApproximator **approx, glucose::IApprox_Parameters_Vector* configuration)
{
	if (*approx_id == line::LineApprox_Descriptor.id)
		return Manufacture_Object<CLine_Approximator>(approx, refcnt::make_shared_reference(signal, true), configuration);

	return ENOENT;
}
