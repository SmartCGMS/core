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
#include "device_event.h"

#include "../../../common/rtl/FilterLib.h"

#include <mutex>
#include <set>

struct TFast_Configuration {
	bool failed = true;
	glucose::SFilter_Chain_Configuration configuration;
	double *first_parameter = nullptr;	//no RC, just for a fast access
};

class CError_Metric_Future {
protected:
	const glucose::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;	
	double mError_Metric = std::numeric_limits<double>::quiet_NaN();
	bool mError_Metric_Available = false;
public:
	CError_Metric_Future(glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) : mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data) {};

	HRESULT On_Filter_Created(glucose::IFilter *filter) {

		if (glucose::SSignal_Error_Inspection insp = glucose::SSignal_Error_Inspection{ glucose::SFilter{filter} }) {
			mError_Metric_Available = insp->Promise_Metric(&mError_Metric, true) == S_OK;
			if (!mError_Metric_Available) return E_FAIL;
		}

		return mOn_Filter_Created(filter, mOn_Filter_Created_Data);
	}

	double Get_Error_Metric() {
		return mError_Metric_Available ? mError_Metric : std::numeric_limits<double>::quiet_NaN();
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
	std::vector<glucose::IDevice_Event*> mEvents_To_Replay;	

	glucose::SFilter_Chain_Configuration Copy_Reduced_Configuration(const size_t end_index) {
		glucose::SFilter_Chain_Configuration reduced_filter_configuration = refcnt::Create_Container_shared<glucose::IFilter_Configuration_Link*, glucose::SFilter_Chain_Configuration>(nullptr, nullptr);

		bool success = true;
		for (size_t link_counter = 0; link_counter < end_index; link_counter++) {
			glucose::SFilter_Configuration_Link single_filter = mConfiguration.operator[](link_counter);
			if (!single_filter) {
				success = false;
				break;
			}
			auto raw_filter = single_filter.get();
			if (reduced_filter_configuration->add(&raw_filter, &raw_filter + 1) != S_OK) {
				success = false;
				break;
			}
		}

		return success ? reduced_filter_configuration : refcnt::Create_Container_shared<glucose::IFilter_Configuration_Link*, glucose::SFilter_Chain_Configuration>(nullptr, nullptr);
	}

	size_t Find_Minimal_Receiver_Index_End() {
		size_t minimal_index_end = 0;
		bool found = false;
		CTerminal_Filter terminal;

		mConfiguration.for_each([this, &terminal, &found, &minimal_index_end](glucose::SFilter_Configuration_Link link) {
			if (found) return;


			GUID id;
			found = link->Get_Filter_Id(&id) != S_OK;
			if (!found) {
				glucose::SFilter filter = create_filter(id, &terminal);
				found = !filter.operator bool();
				if (!found) {
					std::shared_ptr<glucose::IFilter_Feedback_Receiver> feedback_receiver;
					refcnt::Query_Interface<glucose::IFilter, glucose::IFilter_Feedback_Receiver>(filter.get(), glucose::IID_Filter_Feedback_Receiver, feedback_receiver);
					found = feedback_receiver.operator bool();
				}
			}			

			if (!found) minimal_index_end++;
		});

		return minimal_index_end;
	}


	void Fetch_Events_To_Replay() {	
		mEvents_To_Replay.clear();
		const size_t minimal_replay_index_end = std::min(mFilter_Index, Find_Minimal_Receiver_Index_End());
		glucose::SFilter_Chain_Configuration reduced_filter_configuration = Copy_Reduced_Configuration(minimal_replay_index_end);


		std::recursive_mutex communication_guard;
		CComposite_Filter composite_filter{ communication_guard };	//must be in the block that we can precisely 
																		//call its dtor to get the future error properly
		CCopying_Terminal_Filter terminal_filter{ mEvents_To_Replay };
		if (composite_filter.Build_Filter_Chain(reduced_filter_configuration.get(), &terminal_filter, mOn_Filter_Created, mOn_Filter_Created_Data) == S_OK)  terminal_filter.Wait_For_Shutdown(); 
			else mEvents_To_Replay.clear(); //sanitize as this might have been filled partially				
	}
protected:
	const glucose::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;
protected:
	glucose::SFilter_Chain_Configuration mConfiguration;
	const size_t mFilter_Index;
	const std::wstring mParameters_Config_Name;

	TFast_Configuration Clone_Configuration() {
		TFast_Configuration result;
		result.configuration = refcnt::Create_Container_shared<glucose::IFilter_Configuration_Link*, glucose::SFilter_Chain_Configuration>(nullptr, nullptr);

		const std::set<GUID> ui_filters = { glucose::IID_Drawing_Filter, glucose::IID_Log_Filter };	//let's optimize away thos filters, which would only slow down

		//we do not need to do a complete copy -> we just need to 
		// 1. create the root configuration container, because the we can
		// 2. create a new link configuration for the given filter
		// 3. where, we insert new configuration-parameters
		//thus all other objects stays the same, we only increase their reference counters		

		result.failed = false;
		size_t link_counter = 0;
		mConfiguration.for_each([&link_counter, &result, &ui_filters, this](glucose::SFilter_Configuration_Link src_link) {
			if (result.failed) return;

			if ((link_counter < mFilter_Index) && !mEvents_To_Replay.empty()) {
				//we will replay events produced by the first filters
				link_counter++;
				return;
			}


			GUID filter_id = Invalid_GUID;
			if (src_link->Get_Filter_Id(&filter_id) == S_OK) {
				if (ui_filters.find(filter_id) != ui_filters.end()) {
					link_counter++;
					return;
				}
			}

			glucose::IFilter_Configuration_Link* raw_link_to_add = src_link.get();

			if (link_counter == mFilter_Index) {
				if (filter_id == Invalid_GUID) {
					result.failed = true;
					return;
				}
				
				raw_link_to_add = static_cast<glucose::IFilter_Configuration_Link*>(new CFilter_Configuration_Link(filter_id)); //so far, zero RC which is correct right now because we do not call dtor here
				//now, we need to emplace new configuration-parameters

				bool found_parameters = false;
				src_link.for_each([&raw_link_to_add, &found_parameters, &result, this](glucose::SFilter_Parameter src_parameter) {
					
					glucose::IFilter_Parameter* raw_parameter = src_parameter.get();

					if (!found_parameters && (mParameters_Config_Name == src_parameter.configuration_name())) {
						//parameters - we need to create a new copy

						raw_parameter = static_cast<glucose::IFilter_Parameter*>(new CFilter_Parameter{glucose::NParameter_Type::ptDouble_Array, mParameters_Config_Name.c_str()});
						glucose::IModel_Parameter_Vector *src_parameters, *dst_parameters;
						if (src_parameter->Get_Model_Parameters(&src_parameters) == S_OK) {
							dst_parameters = refcnt::Copy_Container<double>(src_parameters);							
							if (raw_parameter->Set_Model_Parameters(dst_parameters) != S_OK) 
									result.failed = true;	//increases RC by 1
							src_parameters->Release();
							dst_parameters->Release();

							//find the very first number to overwrite later on with new values
							double *begin, *end;
							if (dst_parameters->get(&begin, &end) == S_OK) {
								result.first_parameter = begin + mProblem_Size;
								found_parameters = true;
							}
							else
								result.failed = true;
						}
						else
							result.failed = true;
					}

					if (raw_link_to_add->add(&raw_parameter, &raw_parameter + 1) != S_OK) 
							result.failed = true;

					
				});

				if (!found_parameters) 
						result.failed = true;	//we need the parameters, because they also include the bounds!

			}

			result.configuration->add(&raw_link_to_add, &raw_link_to_add + 1);	//increases RC by one
			link_counter++;

		});
		
		return result;
	}

	

public:
	CParameters_Optimizer(glucose::IFilter_Chain_Configuration *configuration, const size_t filter_index, const wchar_t *parameters_config_name, glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) :
		mConfiguration(refcnt::make_shared_reference_ext<glucose::SFilter_Chain_Configuration, glucose::IFilter_Chain_Configuration>(configuration, true)),		
		mFilter_Index(filter_index), mParameters_Config_Name(parameters_config_name),
		mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data) {

	}


	~CParameters_Optimizer() {
		for (auto &event : mEvents_To_Replay)
			event->Release();
	}


	HRESULT Optimize(const GUID solver_id, const size_t population_size, const size_t max_generations, solver::TSolver_Progress &progress) {
		glucose::SFilter_Configuration_Link configuration_link_parameters = mConfiguration.operator[](mFilter_Index);
		if (!configuration_link_parameters || !configuration_link_parameters.Read_Parameters(mParameters_Config_Name.c_str(), mLower_Bound, mFound_Parameters, mUpper_Bound))
			return E_INVALIDARG;

		mProblem_Size = mFound_Parameters.size();


		//create initial pool-configuration to determine problem size
		//and to verify that we are able to clone the configuration
		{
			TFast_Configuration configuration = Clone_Configuration();
			if (configuration.failed) return E_FAIL;
		}
		//succeeded, later on, we should push it to the configuration pool here - once it is implemented		

		Fetch_Events_To_Replay();

		const double* default_parameters = mFound_Parameters.data();

		solver::TSolver_Setup solver_setup{
			mProblem_Size,
			mLower_Bound.data(), mUpper_Bound.data(),
			&default_parameters, 1,					//hints
			mFound_Parameters.data(),

			this, internal::Parameters_Fitness_Wrapper,
			max_generations, population_size, std::numeric_limits<double>::min()
		};


		HRESULT rc = solve_generic(&solver_id, &solver_setup, &progress);		

		//eventually, we need to copy the parameters to the original configuration - on success
		if (rc == S_OK)
			rc = configuration_link_parameters.Write_Parameters(mParameters_Config_Name.c_str(), mLower_Bound, mFound_Parameters, mUpper_Bound) ? rc : E_FAIL;		

		return rc;
	}


	

	double Calculate_Fitness(const void* solution) {
		

		TFast_Configuration configuration = Clone_Configuration();	//later on, we will replace this with a pool
		
		//set the experimental parameters
		std::copy(reinterpret_cast<const double*>(solution), reinterpret_cast<const double*>(solution) + mProblem_Size, configuration.first_parameter);


		//Have the means to pickup the final metric
		CError_Metric_Future error_metric_future{ mOn_Filter_Created, mOn_Filter_Created_Data };
		
		
		//run the configuration
		{
			std::recursive_mutex communication_guard;
			CComposite_Filter composite_filter{ communication_guard };	//must be in the block that we can precisely 
																			//call its dtor to get the future error properly
			CTerminal_Filter terminal_filter;

			if (composite_filter.Build_Filter_Chain(configuration.configuration.get(), &terminal_filter, On_Filter_Created_Wrapper, &error_metric_future) != S_OK)
				return std::numeric_limits<double>::quiet_NaN();

			//wait for the result
			if (mEvents_To_Replay.empty()) terminal_filter.Wait_For_Shutdown();		//no pre-calculated events can be replayed=> we need to go the old-fashioned way
				else for (size_t i = 0; i < mEvents_To_Replay.size(); i++) {		//we can replay the pre-calculated events
						glucose::IDevice_Event *event_to_replay = static_cast<glucose::IDevice_Event*> (new CDevice_Event{ mEvents_To_Replay[i] });
						if (composite_filter.Execute(event_to_replay) != S_OK) break;
				}
		}	//calls dtor of the signal error filter, thus filling the future error metric

		//once implemented, we should return the configuration back to the pool

		//pickup the fitness value/error metric and return it
		return error_metric_future.Get_Error_Metric();
	}
};

double IfaceCalling internal::Parameters_Fitness_Wrapper(const void *data, const double *solution) {
	CParameters_Optimizer *fitness = reinterpret_cast<CParameters_Optimizer*>(const_cast<void*>(data));
	return fitness->Calculate_Fitness(solution);
}


HRESULT IfaceCalling optimize_parameters(glucose::IFilter_Chain_Configuration *configuration, const size_t filter_index, const wchar_t *parameters_configuration_name, 
										 glucose::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
									     const GUID *solver_id, const size_t population_size, const size_t max_generations, solver::TSolver_Progress *progress) {

	CParameters_Optimizer optimizer{ configuration, filter_index, parameters_configuration_name, on_filter_created, on_filter_created_data };
	return optimizer.Optimize(*solver_id, population_size, max_generations, *progress);
}