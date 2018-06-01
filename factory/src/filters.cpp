#include "filters.h"

#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/rtl/descriptor_utils.h"
#include "../../../common/rtl/winapi_mapping.h"
#include "../../../common/lang/dstrings.h"

namespace imported {
	const char* rsGet_Filter_Descriptors = "do_get_filter_descriptors";
	const char* rsGet_Metric_Descriptors = "do_get_metric_descriptors";
	const char* rsGet_Model_Descriptors = "do_get_model_descriptors";
	const char* rsGet_Solvers_Descriptors = "do_get_solver_descriptors";
	const char* rsGet_Approx_Descriptors = "do_get_approximator_descriptors";
	const char* rsDo_Create_Filter = "do_create_filter";
	const char* rsDo_Create_Metric = "do_create_metric";
	const char* rsDo_Create_Signal = "do_create_signal";
	const char* rsDo_Solve_Model_Parameters = "do_solve_model_parameters";
	const char* rsDo_Create_Approximator = "do_create_approximator";
}


CLoaded_Filters loaded_filters;

void Load_Libraries() {
	loaded_filters.load_libraries();
}

HRESULT get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {	
	return loaded_filters.get_filter_descriptors(begin, end);
}

HRESULT get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end) {
	return loaded_filters.get_metric_descriptors(begin, end);
}

HRESULT get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end) {
	return loaded_filters.get_model_descriptors(begin, end);
}

HRESULT get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end) {
	return loaded_filters.get_solver_descriptors(begin, end);
}

HRESULT get_approx_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end) {
	return loaded_filters.get_approx_descriptors(begin, end);
}

HRESULT IfaceCalling create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter) {
	return loaded_filters.create_filter(id, input, output, filter);
}

HRESULT IfaceCalling create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) {
	return loaded_filters.create_metric(parameters, metric);
}

HRESULT IfaceCalling create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	return loaded_filters.create_signal(calc_id, segment, signal);
}

HRESULT IfaceCalling solve_model_parameters(const glucose::TSolver_Setup *setup) {
	return loaded_filters.solve_model_parameters(setup);
}

HRESULT IfaceCalling create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApprox_Parameters_Vector* configuration, glucose::IApproximator **approx) {
	return loaded_filters.create_approximator(approx_id, signal, approx, configuration);
}

HRESULT IfaceCalling add_filters(const glucose::TFilter_Descriptor *begin, const glucose::TFilter_Descriptor *end, const glucose::TCreate_Filter create_filter) {
	return loaded_filters.add_filters(begin, end, create_filter);
}

void CLoaded_Filters::load_libraries() {
	auto appdir = Get_Application_Dir();	
	auto allFiles = List_Directory/*<tbb::tbb_allocator<std::wstring>>*/(Path_Append(appdir, rsSolversDir));

	for (const auto& filepath : allFiles) {	

		if (CDynamic_Library::Is_Library(filepath)) {				//just checks the extension
			imported::TLibraryInfo lib;

			if (lib.library.Load(filepath.c_str())) {
				bool lib_used = Resolve_Func<glucose::TCreate_Filter>(lib.create_filter, lib.library, imported::rsDo_Create_Filter);
				lib_used |= Resolve_Func<glucose::TCreate_Metric>(lib.create_metric, lib.library, imported::rsDo_Create_Metric);
				lib_used |= Resolve_Func<glucose::TCreate_Signal>(lib.create_signal, lib.library, imported::rsDo_Create_Signal);
				lib_used |= Resolve_Func<glucose::TSolve_Model_Parameters>(lib.solve_model_parameters, lib.library, imported::rsDo_Solve_Model_Parameters);
				lib_used |= Resolve_Func<glucose::TCreate_Approximator>(lib.create_approximator, lib.library, imported::rsDo_Create_Approximator);

				lib_used |= Load_Descriptors<glucose::TGet_Filter_Descriptors, glucose::TFilter_Descriptor>(mFilter_Descriptors, lib.library, imported::rsGet_Filter_Descriptors);
				lib_used |= Load_Descriptors<glucose::TGet_Metric_Descriptors, glucose::TMetric_Descriptor>(mMetric_Descriptors, lib.library, imported::rsGet_Metric_Descriptors);
				lib_used |= Load_Descriptors<glucose::TGet_Model_Descriptors, glucose::TModel_Descriptor>(mModel_Descriptors, lib.library, imported::rsGet_Model_Descriptors);
				lib_used |= Load_Descriptors<glucose::TGet_Solver_Descriptors, glucose::TSolver_Descriptor>(mSolver_Descriptors, lib.library, imported::rsGet_Solvers_Descriptors);
				lib_used |= Load_Descriptors<glucose::TGet_Approx_Descriptors, glucose::TApprox_Descriptor>(mApprox_Descriptors, lib.library, imported::rsGet_Approx_Descriptors);

			if (lib_used)
					mLibraries.push_back(std::move(lib));
				else
					lib.library.Unload();
			}
		}
	}	
}

HRESULT CLoaded_Filters::add_filters(const glucose::TFilter_Descriptor *begin, const glucose::TFilter_Descriptor *end, const glucose::TCreate_Filter create_filter) {
	if ((begin == end) || (begin == nullptr) || (end == nullptr) || (create_filter == nullptr)) return E_INVALIDARG;
	imported::TLibraryInfo lib;
	lib.create_approximator = nullptr;
	lib.create_signal = nullptr;
	lib.create_metric = nullptr;
	lib.solve_model_parameters = nullptr;

	lib.create_filter = create_filter;

	mLibraries.push_back(std::move(lib));

	std::copy(begin, end, std::back_inserter(mFilter_Descriptors));

	return S_OK;
}


HRESULT CLoaded_Filters::create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter) {
	auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_filter; }; 
	return Call_Func(call_create_filter, id, input, output, filter);
}

HRESULT CLoaded_Filters::create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) {
	auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_metric; }; 
	return Call_Func(call_create_filter, parameters, metric);
}

HRESULT CLoaded_Filters::create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	auto call_create_signal = [](const imported::TLibraryInfo &info) { return info.create_signal; };
	return Call_Func(call_create_signal, calc_id, segment, signal);
}


HRESULT CLoaded_Filters::solve_model_parameters(const glucose::TSolver_Setup *setup) {
	auto call_solve_filter = [](const imported::TLibraryInfo &info) { return info.solve_model_parameters; }; 
	return Call_Func(call_solve_filter, setup);
}

HRESULT CLoaded_Filters::create_approximator(const GUID *approx_id, glucose::ISignal *signal, glucose::IApproximator **approx, glucose::IApprox_Parameters_Vector* configuration) {
	auto call_create_approx = [](const imported::TLibraryInfo &info) { return info.create_approximator; };
	return Call_Func(call_create_approx, approx_id, signal, approx, configuration);
}

HRESULT IfaceCalling CLoaded_Filters::get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	return do_get_descriptors<glucose::TFilter_Descriptor>(mFilter_Descriptors, begin, end);
}

HRESULT IfaceCalling CLoaded_Filters::get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end) {
	return do_get_descriptors<glucose::TMetric_Descriptor>(mMetric_Descriptors, begin, end);
}


HRESULT IfaceCalling CLoaded_Filters::get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end) {
	return do_get_descriptors<glucose::TModel_Descriptor>(mModel_Descriptors, begin, end);
}

HRESULT IfaceCalling CLoaded_Filters::get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end) {
	return do_get_descriptors<glucose::TSolver_Descriptor>(mSolver_Descriptors, begin, end);
}

HRESULT IfaceCalling CLoaded_Filters::get_approx_descriptors(glucose::TApprox_Descriptor **begin, glucose::TApprox_Descriptor **end) {
	return do_get_descriptors<glucose::TApprox_Descriptor>(mApprox_Descriptors, begin, end);
}
