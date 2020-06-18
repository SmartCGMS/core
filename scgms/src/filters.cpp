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

#include "filters.h"
#include "fitness.h"

#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/utils/descriptor_utils.h"
#include "../../../common/utils/winapi_mapping.h"
#include "../../../common/lang/dstrings.h"

namespace imported {
	const char* rsGet_Filter_Descriptors = "do_get_filter_descriptors";
	const char* rsGet_Metric_Descriptors = "do_get_metric_descriptors";
	const char* rsGet_Model_Descriptors = "do_get_model_descriptors";
	const char* rsGet_Solvers_Descriptors = "do_get_solver_descriptors";
	const char* rsGet_Approx_Descriptors = "do_get_approximator_descriptors";
	const char* rsGet_Signal_Descriptors = "do_get_signal_descriptors";
	const char* rsDo_Create_Filter = "do_create_filter";
	const char* rsDo_Create_Metric = "do_create_metric";
	const char* rsDo_Create_Signal = "do_create_signal";
	const char* rsDo_Create_Discrete_model = "do_create_discrete_model";
	const char* rsDo_Solve_Model_Parameters = "do_solve_model_parameters";
	const char* rsDo_Create_Approximator = "do_create_approximator";
	const char* rsDo_Solve_Generic = "do_solve_generic";
}


CLoaded_Filters loaded_filters{};

CLoaded_Filters::CLoaded_Filters() {
	load_libraries();
}

HRESULT IfaceCalling get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {	
	return loaded_filters.get_filter_descriptors(begin, end);
}

HRESULT IfaceCalling get_metric_descriptors(scgms::TMetric_Descriptor **begin, scgms::TMetric_Descriptor **end) {
	return loaded_filters.get_metric_descriptors(begin, end);
}

HRESULT IfaceCalling get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	return loaded_filters.get_model_descriptors(begin, end);
}

HRESULT IfaceCalling get_solver_descriptors(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end) {
	return loaded_filters.get_solver_descriptors(begin, end);
}

HRESULT IfaceCalling get_approx_descriptors(scgms::TApprox_Descriptor **begin, scgms::TApprox_Descriptor **end) {
	return loaded_filters.get_approx_descriptors(begin, end);
}

HRESULT IfaceCalling get_signal_descriptors(scgms::TSignal_Descriptor** begin, scgms::TSignal_Descriptor** end) {
	return loaded_filters.get_signal_descriptors(begin, end);
}

HRESULT IfaceCalling create_filter(const GUID *id, scgms::IFilter* next_filter, scgms::IFilter **filter) {
	return loaded_filters.create_filter(id, next_filter, filter);
}

HRESULT IfaceCalling create_metric(const scgms::TMetric_Parameters *parameters, scgms::IMetric **metric) {
	return loaded_filters.create_metric(parameters, metric);
}

HRESULT IfaceCalling create_signal(const GUID *calc_id, scgms::ITime_Segment *segment, const GUID *approx_id, scgms::ISignal **signal) {
	return loaded_filters.create_signal(calc_id, segment, approx_id, signal);
}

HRESULT IfaceCalling create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	return loaded_filters.create_discrete_model(model_id, parameters, output, model);
}

HRESULT IfaceCalling solve_model_parameters(const scgms::TSolver_Setup *setup) {
	return loaded_filters.solve_model_parameters(setup);
}

HRESULT IfaceCalling solve_generic(const GUID *solver_id, const solver::TSolver_Setup *setup, solver::TSolver_Progress *progress) {
	return loaded_filters.solve_generic(solver_id, setup, progress);
}

HRESULT IfaceCalling create_approximator(const GUID *approx_id, scgms::ISignal *signal, scgms::IApproximator **approx) {
	return loaded_filters.create_approximator(approx_id, signal, approx);
}

HRESULT IfaceCalling add_filters(const scgms::TFilter_Descriptor *begin, const scgms::TFilter_Descriptor *end, const scgms::TCreate_Filter create_filter) {
	return loaded_filters.add_filters(begin, end, create_filter);
}

void CLoaded_Filters::load_libraries() {
#ifndef ANDROID
	const auto filters_dir = Get_Dll_Dir() / std::wstring{rsSolversDir};
#else
	const auto filters_dir = Get_Dll_Dir();
#endif
	for (const auto& dir_entry : filesystem::directory_iterator(filters_dir)) {
		const auto filepath = dir_entry.path();

		if (CDynamic_Library::Is_Library(filepath)) {				//just checks the extension
			imported::TLibraryInfo lib;

			if (lib.library.Load(filepath)) {
				bool lib_used = Resolve_Func<scgms::TCreate_Filter>(lib.create_filter, lib.library, imported::rsDo_Create_Filter);
				
				lib_used |= Resolve_Func<scgms::TCreate_Metric>(lib.create_metric, lib.library, imported::rsDo_Create_Metric);
				lib_used |= Resolve_Func<scgms::TCreate_Signal>(lib.create_signal, lib.library, imported::rsDo_Create_Signal);
				lib_used |= Resolve_Func<scgms::TCreate_Discrete_Model>(lib.create_discrete_model, lib.library, imported::rsDo_Create_Discrete_model);
				lib_used |= Resolve_Func<scgms::TCreate_Approximator>(lib.create_approximator, lib.library, imported::rsDo_Create_Approximator);
				lib_used |= Resolve_Func<solver::TGeneric_Solver>(lib.solve_generic, lib.library, imported::rsDo_Solve_Generic);

				lib_used |= Load_Descriptors<scgms::TGet_Filter_Descriptors, scgms::TFilter_Descriptor>(mFilter_Descriptors, lib.library, imported::rsGet_Filter_Descriptors);
				lib_used |= Load_Descriptors<scgms::TGet_Metric_Descriptors, scgms::TMetric_Descriptor>(mMetric_Descriptors, lib.library, imported::rsGet_Metric_Descriptors);
				lib_used |= Load_Descriptors<scgms::TGet_Model_Descriptors, scgms::TModel_Descriptor>(mModel_Descriptors, lib.library, imported::rsGet_Model_Descriptors);
				lib_used |= Load_Descriptors<scgms::TGet_Solver_Descriptors, scgms::TSolver_Descriptor>(mSolver_Descriptors, lib.library, imported::rsGet_Solvers_Descriptors);
				lib_used |= Load_Descriptors<scgms::TGet_Approx_Descriptors, scgms::TApprox_Descriptor>(mApprox_Descriptors, lib.library, imported::rsGet_Approx_Descriptors);
				lib_used |= Load_Descriptors<scgms::TGet_Signal_Descriptors, scgms::TSignal_Descriptor>(mSignal_Descriptors, lib.library, imported::rsGet_Signal_Descriptors);

				if (lib_used)
					mLibraries.push_back(std::move(lib));
				else
					lib.library.Unload();
			}
		}
	}
}

HRESULT CLoaded_Filters::add_filters(const scgms::TFilter_Descriptor *begin, const scgms::TFilter_Descriptor *end, const scgms::TCreate_Filter create_filter) {
	if ((begin == end) || (begin == nullptr) || (end == nullptr) || (create_filter == nullptr)) return E_INVALIDARG;
	imported::TLibraryInfo lib;	
	lib.create_approximator = nullptr;
	lib.create_signal = nullptr;
	lib.create_metric = nullptr;	
	lib.create_discrete_model = nullptr;
	lib.solve_model_parameters = nullptr;
	lib.create_filter = create_filter;	

	mLibraries.push_back(std::move(lib));

	std::copy(begin, end, std::back_inserter(mFilter_Descriptors));

	return S_OK;
}


HRESULT CLoaded_Filters::create_filter(const GUID *id, scgms::IFilter *next_filter, scgms::IFilter **filter) {
	if ((!id) || (!next_filter)) return E_INVALIDARG;	
	auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_filter; }; 
	return Call_Func(call_create_filter, id, next_filter, filter);
}

HRESULT CLoaded_Filters::create_metric(const scgms::TMetric_Parameters *parameters, scgms::IMetric **metric) {
	auto call_create_metric = [](const imported::TLibraryInfo &info) { return info.create_metric; }; 
	return Call_Func(call_create_metric, parameters, metric);
}

HRESULT CLoaded_Filters::create_signal(const GUID *calc_id, scgms::ITime_Segment *segment, const GUID* approx_id, scgms::ISignal **signal) {
	auto call_create_signal = [](const imported::TLibraryInfo &info) { return info.create_signal; };
	return Call_Func(call_create_signal, calc_id, segment, approx_id, signal);
}

HRESULT CLoaded_Filters::create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	auto call_create_discrete_model = [](const imported::TLibraryInfo &info) { return info.create_discrete_model; };
	return Call_Func(call_create_discrete_model, model_id, parameters, output, model);
}

HRESULT CLoaded_Filters::solve_model_parameters(const scgms::TSolver_Setup *setup) {
	auto call_solve_model_parameters = [](const imported::TLibraryInfo &info) { return info.solve_model_parameters; }; 
	HRESULT rc = Call_Func(call_solve_model_parameters, setup);

	if (rc != S_OK) {
		//let's try to apply the generic filters as well
		scgms::TModel_Descriptor *model = nullptr;
		for (size_t desc_idx = 0; desc_idx < mModel_Descriptors.size(); desc_idx++)
			for (size_t signal_idx = 0; signal_idx < mModel_Descriptors[desc_idx].number_of_calculated_signals; signal_idx++) {
				if (mModel_Descriptors[desc_idx].calculated_signal_ids[signal_idx] == setup->calculated_signal_id) {
					model = &mModel_Descriptors[desc_idx];
					break;
				}
			}


		if (model != nullptr) {
			double *lower_bound, *upper_bound, *begin, *end;

			if ((setup->lower_bound->get(&lower_bound, &end) != S_OK) || (setup->upper_bound->get(&upper_bound, &end) != S_OK)) return E_FAIL;

			std::vector<double*> hints;
			for (size_t i = 0; i < setup->hint_count; i++) {
				if (setup->solution_hints[i]->get(&begin, &end) == S_OK)
					hints.push_back(begin);
			}	
			
			std::vector<double> solution(model->number_of_parameters);
						
			CFitness fitness{ *setup, model->number_of_parameters};

			solver::TSolver_Setup generic_setup{
				model->number_of_parameters,
				lower_bound, upper_bound,
				const_cast<const double**> (hints.data()), hints.size(),
				solution.data(),


				&fitness,
				Fitness_Wrapper,

				solver::Default_Solver_Setup.max_generations,
				solver::Default_Solver_Setup.population_size,
				solver::Default_Solver_Setup.tolerance,
			};
			
			rc = solve_generic(&setup->solver_id, &generic_setup, setup->progress);
			if (rc == S_OK) {
				rc = setup->solved_parameters->set(solution.data(), solution.data()+solution.size());
			}

			return rc;
		}

	}

	return rc;
}

HRESULT CLoaded_Filters::solve_generic(const GUID *solver_id, const solver::TSolver_Setup *setup, solver::TSolver_Progress *progress) {
	auto call_solve_filter = [](const imported::TLibraryInfo &info) { return info.solve_generic; };
	return Call_Func(call_solve_filter, solver_id, setup, progress);
}

HRESULT CLoaded_Filters::create_approximator(const GUID *approx_id, scgms::ISignal *signal, scgms::IApproximator **approx) {
	auto call_create_approx = [](const imported::TLibraryInfo &info) { return info.create_approximator; };
	return Call_Func(call_create_approx, approx_id, signal, approx);
}

HRESULT CLoaded_Filters::get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	return do_get_descriptors<scgms::TFilter_Descriptor>(mFilter_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_metric_descriptors(scgms::TMetric_Descriptor **begin, scgms::TMetric_Descriptor **end) {
	return do_get_descriptors<scgms::TMetric_Descriptor>(mMetric_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	return do_get_descriptors<scgms::TModel_Descriptor>(mModel_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_solver_descriptors(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end) {
	return do_get_descriptors<scgms::TSolver_Descriptor>(mSolver_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_approx_descriptors(scgms::TApprox_Descriptor **begin, scgms::TApprox_Descriptor **end) {
	return do_get_descriptors<scgms::TApprox_Descriptor>(mApprox_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_signal_descriptors(scgms::TSignal_Descriptor** begin, scgms::TSignal_Descriptor** end) {
	return do_get_descriptors<scgms::TSignal_Descriptor>(mSignal_Descriptors, begin, end);
}

void CLoaded_Filters::describe_loaded_filters(refcnt::Swstr_list error_description) {
	std::wstring desc = dsDefault_Filters_Path;	
	auto appdir = Get_Application_Dir();
	desc += (appdir / std::wstring{ rsSolversDir }).wstring();
	
	error_description.push(desc);
	error_description.push(dsLoaded_Filters);

	if (!mLibraries.empty()) {
		for (auto& lib : mLibraries)
			error_description.push(lib.library.Lib_Path().wstring());
	}
	else
		error_description.push(dsNone);
}

scgms::SFilter create_filter(const GUID &id, scgms::IFilter *next_filter) {
	scgms::SFilter result;
	scgms::IFilter *filter;
	

	if (loaded_filters.create_filter(&id, next_filter, &filter) == S_OK)
		result = refcnt::make_shared_reference_ext<scgms::SFilter, scgms::IFilter>(filter, false);

	return result;
}


void describe_loaded_filters(refcnt::Swstr_list error_description) {
	loaded_filters.describe_loaded_filters(error_description);
}