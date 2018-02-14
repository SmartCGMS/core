#include "filters.h"

#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/rtl/descriptor_utils.h"

namespace imported {
	const char* rsGet_Filter_Descriptors = "do_get_filter_descriptors";
	const char* rsGet_Metric_Descriptors = "do_get_metric_descriptors";
	const char* rsGet_Model_Descriptors = "do_get_model_descriptors";
	const char* rsGet_Solvers_Descriptors = "do_get_solvers_descriptors";
	const char* rsDo_Create_Filter = "do_create_filter";
	const char* rsDo_Create_Metric = "do_create_metric";
	const char* rsDo_Create_Calculated_Signal = "do_create_calculated_signal";
	const char* rsDo_Solve_Model_Parameters = "do_solve_model_parameters";
}

const wchar_t* rsSolversDir = L"filters";

extern "C" char __ImageBase;


CLoaded_Filters loaded_filters;



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

HRESULT IfaceCalling create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter) {
	return loaded_filters.create_filter(id, input, output, filter);
}

HRESULT IfaceCalling create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) {
	return loaded_filters.create_metric(parameters, metric);
}

HRESULT IfaceCalling create_calculated_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	return loaded_filters.create_calculated_signal(calc_id, segment, signal);
}

HRESULT IfaceCalling solve_model_parameters(const glucose::TSolver_Setup *setup) {
	return loaded_filters.solve_model_parameters(setup);
}

void CLoaded_Filters::load_libraries() {
	const auto allFiles = List_Directory(Path_Append(Get_Application_Dir(), rsSolversDir));

	for (const auto& filepath : allFiles) {
		if (CDynamic_Library::Is_Library(filepath)) {				//just checks the extension
			imported::TLibraryInfo lib;

			lib.library = std::make_unique<CDynamic_Library>();
			lib.library->Set_Filename(filepath);

			if (lib.library->Load()) {
				bool lib_used = Resolve_Func<glucose::TCreate_Filter>(lib.create_filter, lib.library, imported::rsDo_Create_Filter);
				lib_used |= Resolve_Func<glucose::TCreate_Metric>(lib.create_metric, lib.library, imported::rsDo_Create_Metric);
				lib_used |= Resolve_Func<glucose::TCreate_Calculated_Signal>(lib.create_calculated_signal, lib.library, imported::rsDo_Create_Calculated_Signal);
				lib_used |= Resolve_Func<glucose::TSolve_Model_Parameters>(lib.solve_model_parameters, lib.library, imported::rsDo_Solve_Model_Parameters);								

				lib_used |= Load_Descriptors<glucose::TGet_Filter_Descriptors, glucose::TFilter_Descriptor>(mFilter_Descriptors, lib.library, imported::rsGet_Filter_Descriptors);
				lib_used |= Load_Descriptors<glucose::TGet_Metric_Descriptors, glucose::TMetric_Descriptor>(mMetric_Descriptors, lib.library, imported::rsGet_Metric_Descriptors);
				lib_used |= Load_Descriptors<glucose::TGet_Model_Descriptors, glucose::TModel_Descriptor>(mModel_Descriptors, lib.library, imported::rsGet_Model_Descriptors);
				lib_used |= Load_Descriptors<glucose::TGet_Solver_Descriptors, glucose::TSolver_Descriptor>(mSolver_Descriptors, lib.library, imported::rsGet_Solvers_Descriptors);

		
			
				if (lib_used)
					mLibraries.push_back(std::move(lib));
				else
					lib.library->Unload();

			}
		}
	}

}


HRESULT CLoaded_Filters::create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter) {
	auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_filter; }; 
	return Call_Func(call_create_filter, id, input, output, filter);
}

HRESULT CLoaded_Filters::create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) {
	auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_metric; }; 
	return Call_Func(call_create_filter, parameters, metric);
}

HRESULT CLoaded_Filters::create_calculated_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_calculated_signal; };
	return Call_Func(call_create_filter, calc_id, segment, signal);
}

HRESULT CLoaded_Filters::solve_model_parameters(const glucose::TSolver_Setup *setup) {
	auto call_solve_filter = [](const imported::TLibraryInfo &info) { return info.solve_model_parameters; }; 
	return Call_Func(call_solve_filter, setup);
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

