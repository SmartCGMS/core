#include "descriptor.h"

#include "errors.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"

const std::vector < glucose::TMetric_Descriptor > metric_descriptor {
	{ mtrAvg_Abs, dsAvg_Abs },
	{ mtrMax_Abs, dsMax_Abs },
	{ mtrPerc_Abs, dsPerc_Abs },
	{ mtrThresh_Abs, dsThresh_Abs },
	{ mtrLeal_2010, dsLeal_2010 },
	{ mtrAIC, dsAIC },
	{ mtrStd_Dev, dsStd_Dev },
	{ mtrCrosswalk, dsCrosswalk },
	{ mtrIntegral_CDF, dsIntegral_CDF },
	{ mtrAvg_Plus_Bessel_Std_Dev, dsAvg_Plus_Bessel_Std_Dev }
};

HRESULT IfaceCalling do_get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end) {
	return do_get_descriptors(metric_descriptor, begin, end);
}

namespace errors
{
	constexpr size_t param_count = 0;

	const glucose::TFilter_Descriptor Errors_Descriptor = {
		glucose::Error_Filter,
		dsErrors_Filter,
		param_count,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { errors::Errors_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == errors::Errors_Descriptor.id)
		return Manufacture_Object<CErrors_Filter>(filter, input, output);

	return ENOENT;
}