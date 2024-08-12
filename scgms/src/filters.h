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

#pragma once

#include <scgms/iface/SolverIface.h>
#include <scgms/iface/UIIface.h>
#include <scgms/iface/ApproxIface.h>
#include <scgms/rtl/Dynamic_Library.h>
#include <scgms/rtl/FilterLib.h>

namespace imported {
	struct TLibraryInfo {
		CDynamic_Library library{};
		scgms::TCreate_Filter create_filter = nullptr;
		scgms::TCreate_Metric create_metric = nullptr;
		scgms::TCreate_Signal create_signal = nullptr;
		scgms::TCreate_Approximator create_approximator = nullptr;
		scgms::TCreate_Discrete_Model create_discrete_model = nullptr;
		solver::TGeneric_Solver solve_generic = nullptr;
	};
}

class CLoaded_Filters {
	protected:
		std::vector<scgms::TFilter_Descriptor> mFilter_Descriptors;
		std::vector<scgms::TMetric_Descriptor> mMetric_Descriptors;
		std::vector<scgms::TModel_Descriptor> mModel_Descriptors;
		std::vector<scgms::TSolver_Descriptor> mSolver_Descriptors;
		std::vector<scgms::TApprox_Descriptor> mApprox_Descriptors;
		std::vector<scgms::TSignal_Descriptor> mSignal_Descriptors;

		std::vector<imported::TLibraryInfo> mLibraries;

	protected:
		template <typename TDesc_Func, typename TDesc_Item>
		bool Load_Descriptors(std::vector<TDesc_Item> &dst, CDynamic_Library &lib, const char *func_name) {
			bool result = false;
			//try to load filter descriptions just once
			TDesc_Func desc_func = reinterpret_cast<decltype(desc_func)> (lib.Resolve(func_name));

			TDesc_Item *desc_begin = nullptr, *desc_end = nullptr;

			if ((desc_func) && (desc_func(&desc_begin, &desc_end) == S_OK)) {
				result = desc_begin != desc_end;
				if (result) {
					std::copy(desc_begin, desc_end, std::back_inserter(dst));
				}
			}

			return result;
		}

		template <typename TFunc>
		bool Resolve_Func(TFunc &ptr, CDynamic_Library &lib, const char* name) {
			ptr = reinterpret_cast<TFunc> (lib.Resolve(name));
			return ptr != nullptr;
		}

		template <typename functype, typename... Args>
		HRESULT Call_Func(functype funcegetter, Args... args) const {
			HRESULT rc = E_NOTIMPL;	//not found

			for (const auto &iter : mLibraries) {
				auto funcptr = funcegetter(iter);
				if (funcptr != nullptr) {
					HRESULT local_rc = (funcptr)(args...);
					if (local_rc == S_OK) {
						return S_OK;
					}

					//else rc remembers the failure code, we so far keep trying to find another library
					//for which the call may succeed, thus eventually seting rc to S_OK
					if (local_rc != E_NOTIMPL) {
						//no need to replace possibly meaningful rc with E_NOTIMPL
						rc = local_rc;
					}
				}
			}
			return rc;
		}

		void load_libraries();
	public:
		CLoaded_Filters();

		//all these methods have the _body suffixes so that linker can safely identify export functions and to be confused with object methods
		HRESULT create_filter_body(const GUID *id, scgms::IFilter *next_filter, scgms::IFilter **filter);
		HRESULT create_metric_body(const scgms::TMetric_Parameters *parameters, scgms::IMetric **metric);
		HRESULT create_signal_body(const GUID *calc_id, scgms::ITime_Segment *segment, const GUID* approx_id, scgms::ISignal **signal);	
		HRESULT solve_generic_body(const GUID *solver_id, const solver::TSolver_Setup *setup, solver::TSolver_Progress *progress);
		HRESULT create_approximator_body(const GUID *approx_id, scgms::ISignal *signal, scgms::IApproximator **approx);
		HRESULT create_discrete_model_body(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model);

		HRESULT get_filter_descriptors_body(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end);
		HRESULT get_metric_descriptors_body(scgms::TMetric_Descriptor **begin, scgms::TMetric_Descriptor **end);
		HRESULT get_model_descriptors_body(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
		HRESULT get_solver_descriptors_body(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end);
		HRESULT get_approx_descriptors_body(scgms::TApprox_Descriptor **begin, scgms::TApprox_Descriptor **end);
		HRESULT get_signal_descriptors_body(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end);
	
		void describe_loaded_filters(refcnt::Swstr_list error_description);
		GUID Resolve_Signal_By_Name(const wchar_t* name, bool& valid);
};

DLL_EXPORT HRESULT IfaceCalling solve_generic(const GUID * solver_id, const solver::TSolver_Setup * setup, solver::TSolver_Progress * progress);

scgms::SFilter create_filter_body(const GUID &id, scgms::IFilter *next_filter);
void describe_loaded_filters(refcnt::Swstr_list error_description);
GUID resolve_signal_by_name(const wchar_t* name, bool& valid);
