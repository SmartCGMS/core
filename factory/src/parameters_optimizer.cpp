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

#include "parameters_optimizer.h"
#include "configuration_link.h"
#include "filter_parameter.h"
#include "filters.h"
#include "executor.h"
#include "composite_filter.h"

#include "../../../common/rtl/FilterLib.h"

#include <mutex>

struct TFast_Configuration {
	bool failed;
	glucose::SFilter_Chain_Configuration configuration;
	double *first_parameter;	//no RC, just for a fast access
};

class CError_Metric_Future {
protected:
	const glucose::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;	
	double mError_Metric;
public:
	CError_Metric_Future(glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) : mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data) {};

	HRESULT On_Filter_Created(glucose::IFilter *filter) {

		if (glucose::SSignal_Error_Inspection insp = glucose::SSignal_Error_Inspection{ glucose::SFilter{filter} }) {
			insp->Promise_Metric(&mError_Metric, true);
		}

		return mOn_Filter_Created(filter, mOn_Filter_Created_Data);
	}

	double Get_Error_Metric() {
		return mError_Metric;
	}
};

HRESULT IfaceCalling On_Filter_Created_Wrapper(glucose::IFilter *filter, const void* data) {
	CError_Metric_Future* on_filter_created = static_cast<CError_Metric_Future*>(const_cast<void*>(data));
	return on_filter_created->On_Filter_Created(filter);
}

class  CParameters_Optimizer {
protected:
	size_t mProblem_Size = 0;
	std::vector<double> mLower_Bound, mUpper_Bound, mFound_Parameters;
protected:
	const glucose::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;
protected:
	glucose::SFilter_Chain_Configuration mConfiguration;
	const size_t mFilter_Index;
	const std::wstring mParameters_Config_Name;

	TFast_Configuration Clone_Configuration() {
		TFast_Configuration result;

		//we do not need to do a complete copy -> we just need to 
		// 1. create the root configuration container, because the we can
		// 2. create a new link configuration for the given filter
		// 3. where, we insert new configuration-parameters
		//thus all other objects stays the same, we only increase their reference counters

		result.failed = false;
		size_t link_counter = 0;
		mConfiguration.for_each([&link_counter, &result, this](glucose::SFilter_Configuration_Link src_link) {
			if (result.failed) return;

			glucose::IFilter_Configuration_Link* raw_link_to_add = src_link.get();

			if (link_counter == mFilter_Index) {
				GUID id = Invalid_GUID;
				if (src_link->Get_Filter_Id(&id) != S_OK) {
					result.failed = true;	
					return;
				}

				raw_link_to_add = static_cast<glucose::IFilter_Configuration_Link*>(new CFilter_Configuration_Link(id)); //so far, zero RC which is correct right now because we do not call dtor here
				//now, we need to emplace new configuration-parameters

				bool found_parameters = false;
				src_link.for_each([&raw_link_to_add, &result, this](glucose::SFilter_Parameter src_parameter) {
					wchar_t *src_name;
					if (src_parameter->Get_Config_Name(&src_name) == S_OK) {

						glucose::IFilter_Parameter* raw_parameter = src_parameter.get();

						if (mParameters_Config_Name == src_name) {
							raw_parameter = static_cast<glucose::IFilter_Parameter*>(new CFilter_Parameter{glucose::NParameter_Type::ptDouble_Array, src_name});
							glucose::IModel_Parameter_Vector *src_parameters, *dst_parameters;
							if (src_parameter->Get_Model_Parameters(&src_parameters) == S_OK) {
								dst_parameters = refcnt::Copy_Container<double>(src_parameters);
								if (raw_parameter->Set_Model_Parameters(dst_parameters) != S_OK) result.failed = true;	//increases RC by 1
								dst_parameters->Release();

								//find the very first number to overwrite later on with new values
								double *begin, *end;
								if (dst_parameters->get(&begin, &end) == S_OK) {
									result.first_parameter = begin + mProblem_Size;
								}
								else
									result.failed = true;
							}
							else
								result.failed = true;
						}

						if (raw_link_to_add->add(&raw_parameter, &raw_parameter + 1) != S_OK) result.failed = true;

					}
				});

				if (!found_parameters) result.failed = true;	//we need the parameters, because they also include the bounds!

			}

			result.configuration->add(&raw_link_to_add, &raw_link_to_add + 1);	//increases RC by one
			link_counter++;

		});

		return result;
	}

	//static so that we can use to to initialize the const member-variable mProblem_Size
	HRESULT Initialize_Solver_Structures() {		

		//OK, let's determine the problem size from the parameters
		glucose::IFilter_Configuration_Link **link_begin, **link_end;
		HRESULT rc = mConfiguration->get(&link_begin, &link_end);
		if (rc != S_OK) return rc;

		glucose::SFilter_Configuration_Link link_parameters = refcnt::make_shared_reference_ext<glucose::SFilter_Configuration_Link, glucose::IFilter_Configuration_Link>(*link_begin + mFilter_Index, true);
		glucose::SModel_Parameter_Vector lower_bound, default_parameters, upper_bound;
		link_parameters.Read_Parameters(mParameters_Config_Name.c_str(), lower_bound, default_parameters, upper_bound);
		double *param_begin, *param_end;
		rc = default_parameters->get(&param_begin, &param_end);
		if (rc != S_OK) return rc;

		mProblem_Size = std::distance(param_begin, param_end);
		if (mProblem_Size == 0) return S_FALSE;


		mLower_Bound = refcnt::Container_To_Vector(lower_bound.get());		
		mUpper_Bound = refcnt::Container_To_Vector(upper_bound.get());
		mFound_Parameters = refcnt::Container_To_Vector(default_parameters.get());

		return S_OK;
	}

public:
	CParameters_Optimizer(glucose::IFilter_Chain_Configuration *configuration, const size_t filter_index, const wchar_t *parameters_config_name, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) :
		mConfiguration(refcnt::make_shared_reference_ext<glucose::SFilter_Chain_Configuration, glucose::IFilter_Chain_Configuration>(configuration, true)),		
		mFilter_Index(filter_index), mParameters_Config_Name(parameters_config_name),
		mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data) {

	}

	HRESULT Optimize(const GUID solver_id, const size_t population_size, const size_t max_generations, solver::TSolver_Progress &progress) {
		HRESULT rc = Initialize_Solver_Structures();
		if (rc != S_OK) return rc;

		//create initial pool-configuration to determine problem size
		//and to verify that we are able to clone the configuration
		TFast_Configuration configuration = Clone_Configuration();
		if (configuration.failed) return E_FAIL;

		//succeeded, later on, we should push it to the configuration pool here - once it is implemented
		const double* default_parameters = mFound_Parameters.data();

		solver::TSolver_Setup solver_setup{
			mProblem_Size,
			mLower_Bound.data(), mUpper_Bound.data(),
			&default_parameters, 1,					//hints
			mFound_Parameters.data(),

			this, internal::Parameters_Fitness_Wrapper,
			max_generations, population_size, std::numeric_limits<double>::min()
		};


		return solve_generic(&solver_id, &solver_setup, &progress);
	}


	

	double Calculate_Fitness(const void* solution) {
		

		TFast_Configuration configuration = Clone_Configuration();	//later on, we will replace this with a pool

		//set the experimental parameters
		std::copy(reinterpret_cast<const double*>(solution), reinterpret_cast<const double*>(solution) + mProblem_Size, configuration.first_parameter);


		//Have the means to pickup the final metric
		CError_Metric_Future error_metric_future{ mOn_Filter_Created, mOn_Filter_Created_Data };
		

		//run the configuration
		std::recursive_mutex mCommunication_Guard;
		CComposite_Filter mComposite_Filter{ mCommunication_Guard };
		CTerminal_Filter mTerminal_Filter;
		
		if (mComposite_Filter.Build_Filter_Chain(configuration.configuration.get(), &mTerminal_Filter, On_Filter_Created_Wrapper, &error_metric_future) != S_OK)
			return std::numeric_limits<double>::quiet_NaN();

		//wait for the result
		mTerminal_Filter.Wait_For_Shutdown();		

		//once implemented, we should return the configuration back to the pool

		//pickup the fitness value/error metric and return it
		return error_metric_future.Get_Error_Metric();
	}
};

double IfaceCalling internal::Parameters_Fitness_Wrapper(const void *data, const double *solution) {
	CParameters_Optimizer *fitness = reinterpret_cast<CParameters_Optimizer*>(const_cast<void*>(data));
	return fitness->Calculate_Fitness(solution);
}


HRESULT IfaceCalling Optimize_Parameters(glucose::IFilter_Chain_Configuration *configuration, const size_t filter_index, const wchar_t *parameters_configuration_name, 
										 glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
									     const GUID solver_id, const size_t population_size, const size_t max_generations, solver::TSolver_Progress *progress) {

	CParameters_Optimizer optimizer{ configuration, filter_index, parameters_configuration_name, on_filter_created, on_filter_created_data };
	return optimizer.Optimize(solver_id, population_size, max_generations, *progress);
}