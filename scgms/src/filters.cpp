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

#include "filters.h"

#include <scgms/rtl/FilesystemLib.h>
#include <scgms/utils/descriptor_utils.h>
#include <scgms/utils/winapi_mapping.h>
#include <scgms/lang/dstrings.h>

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
	const char* rsDo_Create_Approximator = "do_create_approximator";
	const char* rsDo_Solve_Generic = "do_solve_generic";
}


CLoaded_Filters loaded_filters{};

CLoaded_Filters::CLoaded_Filters() {
	load_libraries();
}

DLL_EXPORT HRESULT IfaceCalling get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	return loaded_filters.get_filter_descriptors_body(begin, end);
}

DLL_EXPORT HRESULT IfaceCalling get_metric_descriptors(scgms::TMetric_Descriptor **begin, scgms::TMetric_Descriptor **end) {
	return loaded_filters.get_metric_descriptors_body(begin, end);
}

DLL_EXPORT HRESULT IfaceCalling get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	return loaded_filters.get_model_descriptors_body(begin, end);
}

DLL_EXPORT HRESULT IfaceCalling get_solver_descriptors(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end) {
	return loaded_filters.get_solver_descriptors_body(begin, end);
}

DLL_EXPORT HRESULT IfaceCalling get_approx_descriptors(scgms::TApprox_Descriptor **begin, scgms::TApprox_Descriptor **end) {
	return loaded_filters.get_approx_descriptors_body(begin, end);
}

DLL_EXPORT HRESULT IfaceCalling get_signal_descriptors(scgms::TSignal_Descriptor** begin, scgms::TSignal_Descriptor** end) {
	return loaded_filters.get_signal_descriptors_body(begin, end);
}

DLL_EXPORT HRESULT IfaceCalling create_filter(const GUID *id, scgms::IFilter* next_filter, scgms::IFilter **filter) {
	return loaded_filters.create_filter_body(id, next_filter, filter);
}

DLL_EXPORT HRESULT IfaceCalling create_metric(const scgms::TMetric_Parameters *parameters, scgms::IMetric **metric) {
	return loaded_filters.create_metric_body(parameters, metric);
}

DLL_EXPORT HRESULT IfaceCalling create_signal(const GUID *calc_id, scgms::ITime_Segment *segment, const GUID *approx_id, scgms::ISignal **signal) {
	return loaded_filters.create_signal_body(calc_id, segment, approx_id, signal);
}

DLL_EXPORT HRESULT IfaceCalling create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	return loaded_filters.create_discrete_model_body(model_id, parameters, output, model);
}

DLL_EXPORT HRESULT IfaceCalling solve_generic(const GUID *solver_id, const solver::TSolver_Setup *setup, solver::TSolver_Progress *progress) {
	return loaded_filters.solve_generic_body(solver_id, setup, progress);
}

DLL_EXPORT HRESULT IfaceCalling create_approximator(const GUID *approx_id, scgms::ISignal *signal, scgms::IApproximator **approx) {
	return loaded_filters.create_approximator_body(approx_id, signal, approx);
}

void CLoaded_Filters::load_libraries() {
#ifndef ANDROID
	const auto filters_dir = Get_Dll_Dir() / std::wstring{rsSolversDir};
#else
	const auto filters_dir = Get_Dll_Dir();
#endif

	// filters directory must exist and must be a directory
	if (!filesystem::exists(filters_dir) || !filesystem::is_directory(filters_dir)) {
		return;
	}

	for (const auto& dir_entry : filesystem::directory_iterator(filters_dir)) {
		const auto &filepath = dir_entry.path();

		// just checks the platform-dependent extension to filter out unwanted 
		if (CDynamic_Library::Is_Library(filepath)) {
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

				if (lib_used) {
					mLibraries.push_back(std::move(lib));
				}
				else {
					lib.library.Unload();
				}
			}
		}
	}
}


HRESULT CLoaded_Filters::create_filter_body(const GUID *id, scgms::IFilter *next_filter, scgms::IFilter **filter) {
	if ((!id) || (!next_filter)) {
		return E_INVALIDARG;
	}
	auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_filter; }; 
	return Call_Func(call_create_filter, id, next_filter, filter);
}

HRESULT CLoaded_Filters::create_metric_body(const scgms::TMetric_Parameters *parameters, scgms::IMetric **metric) {
	auto call_create_metric = [](const imported::TLibraryInfo &info) { return info.create_metric; }; 
	return Call_Func(call_create_metric, parameters, metric);
}

HRESULT CLoaded_Filters::create_signal_body(const GUID *calc_id, scgms::ITime_Segment *segment, const GUID* approx_id, scgms::ISignal **signal) {
	auto call_create_signal = [](const imported::TLibraryInfo &info) { return info.create_signal; };
	return Call_Func(call_create_signal, calc_id, segment, approx_id, signal);
}

HRESULT CLoaded_Filters::create_discrete_model_body(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	auto call_create_discrete_model = [](const imported::TLibraryInfo &info) { return info.create_discrete_model; };
	return Call_Func(call_create_discrete_model, model_id, parameters, output, model);
}

HRESULT CLoaded_Filters::solve_generic_body(const GUID *solver_id, const solver::TSolver_Setup *setup, solver::TSolver_Progress *progress) {
	auto call_solve_filter = [](const imported::TLibraryInfo &info) { return info.solve_generic; };
	return Call_Func(call_solve_filter, solver_id, setup, progress);
}

HRESULT CLoaded_Filters::create_approximator_body(const GUID *approx_id, scgms::ISignal *signal, scgms::IApproximator **approx) {
	auto call_create_approx = [](const imported::TLibraryInfo &info) { return info.create_approximator; };
	return Call_Func(call_create_approx, approx_id, signal, approx);
}

HRESULT CLoaded_Filters::get_filter_descriptors_body(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	return do_get_descriptors<scgms::TFilter_Descriptor>(mFilter_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_metric_descriptors_body(scgms::TMetric_Descriptor **begin, scgms::TMetric_Descriptor **end) {
	return do_get_descriptors<scgms::TMetric_Descriptor>(mMetric_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_model_descriptors_body(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	return do_get_descriptors<scgms::TModel_Descriptor>(mModel_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_solver_descriptors_body(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end) {
	return do_get_descriptors<scgms::TSolver_Descriptor>(mSolver_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_approx_descriptors_body(scgms::TApprox_Descriptor **begin, scgms::TApprox_Descriptor **end) {
	return do_get_descriptors<scgms::TApprox_Descriptor>(mApprox_Descriptors, begin, end);
}

HRESULT CLoaded_Filters::get_signal_descriptors_body(scgms::TSignal_Descriptor** begin, scgms::TSignal_Descriptor** end) {
	return do_get_descriptors<scgms::TSignal_Descriptor>(mSignal_Descriptors, begin, end);
}

void CLoaded_Filters::describe_loaded_filters(refcnt::Swstr_list error_description) {
	std::wstring desc = dsDefault_Filters_Path;	
	auto appdir = Get_Application_Dir();
	desc += (appdir / std::wstring{ rsSolversDir }).wstring();
	
	error_description.push(desc);
	error_description.push(dsLoaded_Filters);

	if (!mLibraries.empty()) {
		for (auto& lib : mLibraries) {
			error_description.push(lib.library.Lib_Path().wstring());
		}
	}
	else {
		error_description.push(dsNone);
	}
}

GUID CLoaded_Filters::Resolve_Signal_By_Name(const wchar_t* name, bool& valid) {
	valid = false;
	const std::wstring cname{ name };
	for (const auto& elem : mSignal_Descriptors) {
		if (cname.compare(elem.signal_description) == 0) {
			valid = true;
			return elem.id;
		}
	}

	for (size_t i = 0; i < scgms::signal_Virtual.size(); i++) {
		std::wstring desc_str = dsSignal_Prefix_Virtual + std::wstring(L" ") + std::to_wstring(i);
		if (cname.compare(desc_str) == 0) {
			valid = true;
			return scgms::signal_Virtual[i];
		}
	}

	return Invalid_GUID;
}

scgms::SFilter create_filter_body(const GUID &id, scgms::IFilter *next_filter) {
	scgms::SFilter result;
	scgms::IFilter *filter;

	if (loaded_filters.create_filter_body(&id, next_filter, &filter) == S_OK) {
		result = refcnt::make_shared_reference_ext<scgms::SFilter, scgms::IFilter>(filter, false);
	}

	return result;
}

void describe_loaded_filters(refcnt::Swstr_list error_description) {
	loaded_filters.describe_loaded_filters(error_description);
}

GUID resolve_signal_by_name(const wchar_t* name, bool& valid) {
	return loaded_filters.Resolve_Signal_By_Name(name, valid);
}
