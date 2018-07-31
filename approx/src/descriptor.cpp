#include "descriptor.h"
#include "line.h"
#include "Akima.h"

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

namespace akima {
	constexpr size_t param_count = 0;

	const glucose::TApprox_Descriptor Akima_Descriptor = {
		{ 0xc3e9669d, 0x594a, 0x4fd4,{ 0xb0, 0xf4, 0x44, 0xab, 0x9d, 0x4e, 0x7, 0x39 } },		// {C3E9669D-594A-4FD4-B0F4-44AB9D4E0739}
		dsAkima,
		param_count,
		nullptr,
		nullptr
	};
}

const std::array<glucose::TApprox_Descriptor, 2> approx_descriptions = { line::LineApprox_Descriptor, akima::Akima_Descriptor };

extern "C" HRESULT IfaceCalling do_get_approximator_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end) {
	*begin = const_cast<glucose::TApprox_Descriptor*>(approx_descriptions.data());
	*end = *begin + approx_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApprox_Parameters_Vector* configuration, glucose::IApproximator **approx) {

	return Manufacture_Object<CAkima>(approx, glucose::WSignal{ signal }, configuration);

	if (*approx_id == line::LineApprox_Descriptor.id)
		return Manufacture_Object<CLine_Approximator>(approx, glucose::WSignal{ signal }, configuration);
		

	return ENOENT;
}
