#include "factory.h"

#include "AreaUnderCurve.h"
#include "metric.h"

#include "../../../common/rtl/manufactory.h"


HRESULT IfaceCalling DoCreateMetricFactory(size_t factoryid, TMetricParameters params, IMetricFactory **factory) {
	HRESULT rc = E_NOTIMPL;

	switch (factoryid) {
		case mfiAvgAbs:   rc = ManufactureObject<CMetricFactory<CAbsDiffAvgMetric>, IMetricFactory>(factory, params);
						  break;
		
		case mfiMaxAbs:   rc = ManufactureObject<CMetricFactory<CAbsDiffMaxMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiPercAbs:  rc = ManufactureObject<CMetricFactory<CAbsDiffPercentilMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiThreshAbs:rc = ManufactureObject<CMetricFactory<CAbsDiffThresholdMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiLeal2010: rc = ManufactureObject<CMetricFactory<CLeal2010Metric>, IMetricFactory>(factory, params);
						  break;
						
		case mfiAIC:  	  rc = ManufactureObject<CMetricFactory<CAICMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiStdDev:	  rc = ManufactureObject<CMetricFactory<CStdDevMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiCrossWalk:rc = ManufactureObject<CMetricFactory<CCrossWalkMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiIntegralCDF:rc = ManufactureObject<CMetricFactory<CIntegralCDFMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiAvgPlusBesselStdDev:rc = ManufactureObject<CMetricFactory<CAvgPlusBesselStdDevMetric>, IMetricFactory>(factory, params);
						  break;

		case mfiAUC: rc = ManufactureObject<CMetricFactory<CAreaUnderCurve>, IMetricFactory>(factory, params);
			  			  break;

		case mfiPath_Difference: rc = ManufactureObject<CMetricFactory<CPath_Difference>, IMetricFactory>(factory, params);
			break;
	}

	return rc;
}