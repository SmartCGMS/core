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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "metric.h"
#include "descriptor.h"

#include <map>
#include <functional>

#include <scgms/iface/SolverIface.h>
#include <scgms/rtl/manufactory.h>

using TCreate_Metric = std::function<HRESULT(const scgms::TMetric_Parameters &parameters, scgms::IMetric **metric)>;

class CId_Dispatcher {
	protected:
		std::map <const GUID, TCreate_Metric, std::less<GUID>> id_map;

		template <typename T>
		HRESULT Create_X(const scgms::TMetric_Parameters &params, scgms::IMetric **metric) const {
			return Manufacture_Object<T, scgms::IMetric>(metric, params);
		}

		template <typename T>
		inline void Bind_Metric_Factory(const GUID& id) {
			id_map[id] = std::bind(&CId_Dispatcher::Create_X<T>, this, std::placeholders::_1, std::placeholders::_2);
		}

	public:
		CId_Dispatcher() {
			Bind_Metric_Factory<CAbsDiffAvgMetric>(mtrAvg_Abs);
			Bind_Metric_Factory<CAbsDiffMaxMetric>(mtrMax_Abs);
			Bind_Metric_Factory<CAbsDiffPercentilMetric>(mtrPerc_Abs);
			Bind_Metric_Factory<CAbsDiffThresholdMetric>(mtrThresh_Abs);
			Bind_Metric_Factory<CLeal2010Metric>(mtrLeal_2010);
			Bind_Metric_Factory<CAICMetric>(mtrAIC);
			Bind_Metric_Factory<CStdDevMetric>(mtrStd_Dev);
			Bind_Metric_Factory<CVariance_Metric>(mtrVariance);
			Bind_Metric_Factory<CCrossWalkMetric>(mtrCrosswalk);
			Bind_Metric_Factory<CIntegralCDFMetric>(mtrIntegral_CDF);
			Bind_Metric_Factory<CAvgPlusBesselStdDevMetric>(mtrAvg_Plus_Bessel_Std_Dev);
			Bind_Metric_Factory<CRMSE_Metric>(mtrRMSE);
			Bind_Metric_Factory<CExpWeightedDiffAvgPolar_Metric>(mtrExpWtDiff);
			Bind_Metric_Factory<CAvg_Pow_StdDev_Metric>(mtrAvg_Pow_StdDev_Metric);
		}

		HRESULT Create_Metric(const scgms::TMetric_Parameters &parameters, scgms::IMetric **metric) const {
			const auto iter = id_map.find(parameters.metric_id);
			if (iter != id_map.end()) {
				return iter->second(parameters, metric);
			}
			else {
				return E_NOTIMPL;
			}
		}
};

CId_Dispatcher Id_Dispatcher;

DLL_EXPORT HRESULT IfaceCalling do_create_metric(const scgms::TMetric_Parameters *parameters, scgms::IMetric **metric) {
	if (parameters == nullptr) {
		return E_INVALIDARG;
	}
	return Id_Dispatcher.Create_Metric(*parameters, metric);
}
