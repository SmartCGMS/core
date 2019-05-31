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
 * Univerzitni 8
 * 301 00, Pilsen
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

#pragma once

#include "../../../common/iface/SolverIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/iface/ApproxIface.h"
#include "../../../common/iface/SensorIface.h"
#include "../../../common/rtl/Dynamic_Library.h"

#include <tbb/concurrent_vector.h>
#include <tbb/tbb_allocator.h>

namespace imported {
	struct TLibraryInfo {
		CDynamic_Library library{};
		glucose::TCreate_Asynchronnous_Filter create_asynchronnous_filter = nullptr;
		glucose::TCreate_Synchronnous_Filter create_synchronnous_filter = nullptr;
		glucose::TCreate_Metric create_metric = nullptr;
		glucose::TCreate_Signal create_signal = nullptr;
		glucose::TCreate_Device_Driver create_device_driver = nullptr;
		glucose::TCreate_Approximator create_approximator = nullptr;
		glucose::TSolve_Model_Parameters solve_model_parameters = nullptr;
		solver::TGeneric_Solver solve_generic = nullptr;
	};
}

class CLoaded_Filters {
protected:
	//on tbb_allocator, see comment at mLibraries declaration
	std::vector<glucose::TFilter_Descriptor, tbb::tbb_allocator<glucose::TFilter_Descriptor>> mFilter_Descriptors;
	std::vector<glucose::TMetric_Descriptor, tbb::tbb_allocator<glucose::TMetric_Descriptor>> mMetric_Descriptors;
	std::vector<glucose::TModel_Descriptor, tbb::tbb_allocator<glucose::TModel_Descriptor>> mModel_Descriptors;
	std::vector<glucose::TSolver_Descriptor, tbb::tbb_allocator<glucose::TSolver_Descriptor>> mSolver_Descriptors;
	std::vector<glucose::TApprox_Descriptor, tbb::tbb_allocator<glucose::TApprox_Descriptor>> mApprox_Descriptors;
	std::vector<glucose::TDevice_Driver_Descriptor, tbb::tbb_allocator<glucose::TDevice_Driver_Descriptor>> mDevice_Driver_Descriptors;
	
	template <typename TDesc_Func, typename TDesc_Item>
	bool Load_Descriptors(std::vector<TDesc_Item, tbb::tbb_allocator<TDesc_Item>> &dst, CDynamic_Library &lib, const char *func_name)  {
		bool result = false;
		//try to load filter descriptions just once
		TDesc_Func desc_func = reinterpret_cast<decltype(desc_func)> (lib.Resolve(func_name));

		TDesc_Item *desc_begin, *desc_end;

		if ((desc_func) && (desc_func(&desc_begin, &desc_end) == S_OK)) {
			result = desc_begin != desc_end;
			std::copy(desc_begin, desc_end, std::back_inserter(dst));
		}

		return result;
	}
protected:
	//std::vector<imported::TLibraryInfo> mLibraries; - we need to uses tbb allocator due to the pipe's implementation that uses tbb::concurrent_queue
	std::vector<imported::TLibraryInfo, tbb::tbb_allocator<imported::TLibraryInfo>> mLibraries;

	template <typename TFunc>
	bool Resolve_Func(TFunc &ptr, CDynamic_Library &lib, const char* name) {
		ptr = reinterpret_cast<TFunc> (lib.Resolve(name));
		return ptr != nullptr;
	}


	template <typename functype, typename... Args>
	HRESULT Call_Func(functype funcegetter, Args... args) const {
		HRESULT rc = E_NOTIMPL;	//not found

		for (auto &iter : mLibraries) {
			auto funcptr = funcegetter(iter);
			if (funcptr != nullptr) {
				HRESULT local_rc = (funcptr)(args...);
				if (local_rc == S_OK) return S_OK;
					//else rc remembers the failure code, we so far keep trying to find another library
					//for which the call may succeed, thus eventually seting rc to S_OK
				if (local_rc != E_NOTIMPL) rc = local_rc;	//no need to replace possibly meaningful rc with E_NOTIMPL
			}
		}
		return rc;
	}

	void load_libraries();
public:
	CLoaded_Filters();

	HRESULT create_asynchronnous_filter(const GUID *id, glucose::IFilter_Asynchronous_Pipe *input, glucose::IFilter_Asynchronous_Pipe *output, glucose::IAsynchronnous_Filter **filter);
	HRESULT create_synchronnous_filter(const GUID *id, glucose::ISynchronnous_Filter **filter);
	HRESULT create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric);
	HRESULT create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal);
	HRESULT create_device_driver(const GUID *calc_id, glucose::IFilter_Asynchronous_Pipe* output, glucose::IDevice_Driver** device_driver);
	HRESULT solve_model_parameters(const glucose::TSolver_Setup *setup);
	HRESULT solve_generic(const GUID *solver_id, const solver::TSolver_Setup *setup, solver::TSolver_Progress *progress);
	HRESULT create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApprox_Parameters_Vector* configuration, glucose::IApproximator **approx);

	HRESULT get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
	HRESULT get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end);
	HRESULT get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
	HRESULT get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);
	HRESULT get_approx_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end);
	HRESULT get_device_driver_descriptors(glucose::TDevice_Driver_Descriptor **begin, glucose::TDevice_Driver_Descriptor **end);

	HRESULT add_filters(const glucose::TFilter_Descriptor *begin, const glucose::TFilter_Descriptor *end, const glucose::TCreate_Asynchronnous_Filter create_filter, const glucose::TCreate_Synchronnous_Filter create_synchronnous_filter);
};

extern "C" HRESULT IfaceCalling create_asynchronnous_filter(const GUID *id, glucose::IFilter_Asynchronous_Pipe *input, glucose::IFilter_Asynchronous_Pipe *output, glucose::IAsynchronnous_Filter **filter);
extern "C" HRESULT IfaceCalling create_synchronnous_filter(const GUID *id, glucose::ISynchronnous_Filter **filter);
extern "C" HRESULT IfaceCalling create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric);
extern "C" HRESULT IfaceCalling create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal);
extern "C" HRESULT IfaceCalling create_device_driver(const GUID *calc_id, glucose::IFilter_Asynchronous_Pipe* output, glucose::IDevice_Driver** device_driver);
extern "C" HRESULT IfaceCalling solve_model_parameters(const glucose::TSolver_Setup *setup);
extern "C" HRESULT IfaceCalling solve_generic(const GUID *solver_id, const solver::TSolver_Setup *setup, solver::TSolver_Progress *progress);
extern "C" HRESULT IfaceCalling create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApprox_Parameters_Vector* configuration, glucose::IApproximator **approx);

extern "C" HRESULT IfaceCalling get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end);
extern "C" HRESULT IfaceCalling get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);
extern "C" HRESULT IfaceCalling get_approx_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end);
extern "C" HRESULT IfaceCalling get_device_driver_descriptors(glucose::TDevice_Driver_Descriptor **begin, glucose::TDevice_Driver_Descriptor **end);

extern "C" HRESULT IfaceCalling add_filters(const glucose::TFilter_Descriptor *begin, const glucose::TFilter_Descriptor *end, const glucose::TCreate_Asynchronnous_Filter create_filter, const glucose::TCreate_Synchronnous_Filter create_synchronnous_filter);
