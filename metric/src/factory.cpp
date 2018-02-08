#include "factory.h"

#include "metric.h"
#include "descriptor.h"

#include <map>
#include <functional>


#include "../../../common/rtl/manufactory.h"


class CId_Dispatcher {
protected:
	std::map <const GUID, std::function<HRESULT(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric)>> id_map;

	template <typename T>
	HRESULT Create_X(const glucose::TMetric_Parameters *params, glucose::IMetric **metric) const {
		return Manufacture_Object<T, glucose::IMetric>(metric, *params);
	}

public:
	CId_Dispatcher() {
		id_map[mtrAvg_Abs] = std::bind(&CId_Dispatcher::Create_X<CAbsDiffAvgMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrMax_Abs] = std::bind(&CId_Dispatcher::Create_X<CAbsDiffMaxMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrPerc_Abs] = std::bind(&CId_Dispatcher::Create_X<CAbsDiffPercentilMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrThresh_Abs] = std::bind(&CId_Dispatcher::Create_X<CAbsDiffThresholdMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrLeal_2010] = std::bind(&CId_Dispatcher::Create_X<CLeal2010Metric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrAIC] = std::bind(&CId_Dispatcher::Create_X<CAICMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrStd_Dev] = std::bind(&CId_Dispatcher::Create_X<CStdDevMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrCrosswalk] = std::bind(&CId_Dispatcher::Create_X<CCrossWalkMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrIntegral_CDF] = std::bind(&CId_Dispatcher::Create_X<CIntegralCDFMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrAvg_Plus_Bessel_Std_Dev] = std::bind(&CId_Dispatcher::Create_X<CAvgPlusBesselStdDevMetric>, this, std::placeholders::_1, std::placeholders::_2);
	}

	HRESULT Create_Metric(const GUID *metric_id, const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) const {
		const auto iter = id_map.find(*metric_id);
		if (iter != id_map.end())
			return iter->second(parameters, metric);
		else return E_NOTIMPL;
	}
};



static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_create_metric(const GUID *metric_id, const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) {
	return Id_Dispatcher.Create_Metric(metric_id, parameters, metric);
}