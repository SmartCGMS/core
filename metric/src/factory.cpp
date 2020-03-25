/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "factory.h"

#include "metric.h"
#include "descriptor.h"

#include <map>
#include <functional>

#include "../../../common/rtl/manufactory.h"

using TCreate_Metric = std::function<HRESULT(const scgms::TMetric_Parameters &parameters, scgms::IMetric **metric)>;

class CId_Dispatcher {
protected:
	std::map <const GUID, TCreate_Metric, std::less<GUID>> id_map;

	template <typename T>
	HRESULT Create_X(const scgms::TMetric_Parameters &params, scgms::IMetric **metric) const {
		return Manufacture_Object<T, scgms::IMetric>(metric, params);
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
		id_map[mtrVariance] = std::bind(&CId_Dispatcher::Create_X<CVariance_Metric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrCrosswalk] = std::bind(&CId_Dispatcher::Create_X<CCrossWalkMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrIntegral_CDF] = std::bind(&CId_Dispatcher::Create_X<CIntegralCDFMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrAvg_Plus_Bessel_Std_Dev] = std::bind(&CId_Dispatcher::Create_X<CAvgPlusBesselStdDevMetric>, this, std::placeholders::_1, std::placeholders::_2);
		id_map[mtrRMSE] = std::bind(&CId_Dispatcher::Create_X<CRMSE_Metric>, this, std::placeholders::_1, std::placeholders::_2);		
	}

	HRESULT Create_Metric(const scgms::TMetric_Parameters &parameters, scgms::IMetric **metric) const {
		const auto iter = id_map.find(parameters.metric_id);
		if (iter != id_map.end())
			return iter->second(parameters, metric);
		else return E_NOTIMPL;
	}
};

CId_Dispatcher Id_Dispatcher;

HRESULT IfaceCalling do_create_metric(const scgms::TMetric_Parameters *parameters, scgms::IMetric **metric) {
	if (parameters == nullptr) return E_INVALIDARG;
	return Id_Dispatcher.Create_Metric(*parameters, metric);
}
