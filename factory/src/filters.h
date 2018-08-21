#pragma once

#include "../../../common/iface/SolverIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/iface/ApproxIface.h"
#include "../../../common/rtl/Dynamic_Library.h"

#include <tbb/concurrent_vector.h>
#include <tbb/tbb_allocator.h>

namespace imported {
	struct TLibraryInfo {	
		CDynamic_Library library{};
		glucose::TCreate_Filter create_filter = nullptr;
		glucose::TCreate_Metric create_metric = nullptr;
		glucose::TCreate_Signal create_signal = nullptr;		
		glucose::TSolve_Model_Parameters solve_model_parameters = nullptr;
		glucose::TCreate_Approximator create_approximator = nullptr;
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

	HRESULT create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter);
	HRESULT create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric);
	HRESULT create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal);
	HRESULT solve_model_parameters(const glucose::TSolver_Setup *setup);
	HRESULT create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApprox_Parameters_Vector* configuration, glucose::IApproximator **approx);

	HRESULT get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
	HRESULT get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end);
	HRESULT get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
	HRESULT get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);
	HRESULT get_approx_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end);

	HRESULT add_filters(const glucose::TFilter_Descriptor *begin, const glucose::TFilter_Descriptor *end, const glucose::TCreate_Filter create_filter);
};

extern "C" HRESULT IfaceCalling create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter);
extern "C" HRESULT IfaceCalling create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric);
extern "C" HRESULT IfaceCalling create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal);
extern "C" HRESULT IfaceCalling solve_model_parameters(const glucose::TSolver_Setup *setup);
extern "C" HRESULT IfaceCalling create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApprox_Parameters_Vector* configuration, glucose::IApproximator **approx);

extern "C" HRESULT IfaceCalling get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end);
extern "C" HRESULT IfaceCalling get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end);
extern "C" HRESULT IfaceCalling get_approx_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end);

extern "C" HRESULT IfaceCalling add_filters(const glucose::TFilter_Descriptor *begin, const glucose::TFilter_Descriptor *end, const glucose::TCreate_Filter create_filter);