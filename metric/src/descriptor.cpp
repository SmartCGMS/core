#include "descriptor.h"

#include "errors.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"

const std::array < glucose::TMetric_Descriptor, 10 > metric_descriptor {
	 glucose::TMetric_Descriptor {mtrAvg_Abs, dsAvg_Abs } ,
	 glucose::TMetric_Descriptor{ mtrMax_Abs, dsMax_Abs },
	 glucose::TMetric_Descriptor{ mtrPerc_Abs, dsPerc_Abs },
	 glucose::TMetric_Descriptor{ mtrThresh_Abs, dsThresh_Abs },
	 glucose::TMetric_Descriptor{ mtrLeal_2010, dsLeal_2010 },
	 glucose::TMetric_Descriptor{ mtrAIC, dsAIC },
	 glucose::TMetric_Descriptor{ mtrStd_Dev, dsStd_Dev },
	 glucose::TMetric_Descriptor{ mtrCrosswalk, dsCrosswalk },
	 glucose::TMetric_Descriptor{ mtrIntegral_CDF, dsIntegral_CDF },
	 glucose::TMetric_Descriptor{ mtrAvg_Plus_Bessel_Std_Dev, dsAvg_Plus_Bessel_Std_Dev }
};

HRESULT IfaceCalling do_get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end) {
	*begin = const_cast<glucose::TMetric_Descriptor*>(metric_descriptor.data());
	*end = *begin + metric_descriptor.size();
	return S_OK;
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

static const std::array<glucose::TFilter_Descriptor, 1> filter_descriptions = { errors::Errors_Descriptor };

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