#include "descriptor.h"

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