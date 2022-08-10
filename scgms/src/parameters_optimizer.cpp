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

#include "parameters_optimizer.h"
#include "configuration_link.h"
#include "filter_parameter.h"
#include "filters.h"
#include "executor.h"
#include "composite_filter.h"
#include "device_event.h"
#include "persistent_chain_configuration.h"

#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/utils/DebugHelper.h"
#include "../../../common/lang/dstrings.h"

#include <mutex>
#include <set>
#include <numeric>
#include <execution> 

class CNull_wstr_list : public refcnt::Swstr_list {
		//hides all the methods to do nothing
		//i.e.; to even save the overhead of checking the null pointer as Swstr_list does
public:
	CNull_wstr_list() : SReferenced<refcnt::wstr_list>{ } {};
	void push(const wchar_t* wstr) {};
	void push(const std::wstring& wstr) {};

	void for_each(std::function<void(const std::wstring& wstr)> callback) const {};
};

struct TFast_Configuration {
	bool failed = true;
	scgms::SFilter_Chain_Configuration configuration;
	std::vector<double*> first_parameter_ptrs;
};

class CError_Metric_Future {
protected:
	const scgms::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;		
protected:
	//bool mError_Metric_Available = false;
	//double mError_Metric = std::numeric_limits<double>::quiet_NaN();
	solver::TFitness mError_Metric = solver::Nan_Fitness;
		//has to be array as vector could actually reallocate the memory block on push_back
	size_t mError_Metric_Count = 0;	//count of all used metrics
public:
	CError_Metric_Future(scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data) : mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data) {};

	HRESULT On_Filter_Created(scgms::IFilter *filter) {

		if (mError_Metric_Count < mError_Metric.size()) {	//check if we actually have a room to store the promised metric
			scgms::SSignal_Error_Inspection insp = scgms::SSignal_Error_Inspection{ scgms::SFilter{filter} };
			if (insp) {
				const bool metric_available = insp->Promise_Metric(scgms::All_Segments_Id, &mError_Metric[mError_Metric_Count], true) == S_OK;
				if (metric_available)
					mError_Metric_Count++;
				else
					return E_FAIL;

			}
		}

		return mOn_Filter_Created ? mOn_Filter_Created(filter, mOn_Filter_Created_Data) : S_OK;
	}

	size_t Get_Error_Metric(double * const fitness) const {
		for (size_t i = 0; i < mError_Metric_Count; i++)	//we have to copy this in a reverse to order to make highest priority first
			fitness[i] = mError_Metric[mError_Metric_Count - i - 1];
		
		return mError_Metric_Count;		
	}

	size_t Metric_Count() const {
		return mError_Metric_Count;
	}
};

HRESULT IfaceCalling On_Filter_Created_Wrapper(scgms::IFilter *filter, const void* data) {
	CError_Metric_Future* on_filter_created = static_cast<CError_Metric_Future*>(const_cast<void*>(data));
	return on_filter_created->On_Filter_Created(filter);
}

class  CParameters_Optimizer {
protected:
	size_t mProblem_Size = 0;
	size_t mObjectives_Count = 0;
	std::vector<double> mLower_Bound, mUpper_Bound, mFound_Parameters;
protected:
	scgms::SFilter_Chain_Configuration Empty_Chain() {
		auto result = refcnt::Manufacture_Object_Shared<CPersistent_Chain_Configuration, scgms::IFilter_Chain_Configuration, scgms::SFilter_Chain_Configuration>();

		refcnt::wstr_container* parent_path = nullptr;
		if (Succeeded(mConfiguration->Get_Parent_Path(&parent_path))) {
                        const auto converted_path = refcnt::WChar_Container_To_WString(parent_path);
                        if (!Succeeded(result->Set_Parent_Path(converted_path.c_str())))
				result.reset();
			parent_path->Release();
		}
		else
			result.reset();

		return result;
	}
protected:
	std::vector<CDevice_Event> mEvents_To_Replay;

	scgms::SFilter_Chain_Configuration Copy_Reduced_Configuration(const size_t end_index) {
					
		scgms::SFilter_Chain_Configuration reduced_filter_configuration = Empty_Chain();
		bool success = reduced_filter_configuration.operator bool();

		if (success) {
			refcnt::wstr_container* parent_path = nullptr;
			success = mConfiguration->Get_Parent_Path(&parent_path) == S_OK;
			if (success) {
                const auto converted_path = refcnt::WChar_Container_To_WString(parent_path);
                success = reduced_filter_configuration->Set_Parent_Path(converted_path.c_str()) == S_OK;
				parent_path->Release();
			}


			for (size_t link_counter = 0; link_counter < end_index; link_counter++) {
				scgms::SFilter_Configuration_Link single_filter = mConfiguration[link_counter];
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
		}

		return success ? reduced_filter_configuration : Empty_Chain();
	}

	size_t Find_Minimal_Receiver_Index_End() {
		size_t minimal_index_end = 0;
		bool found = false;
		CTerminal_Filter terminal{ nullptr };

		mConfiguration.for_each([&terminal, &found, &minimal_index_end](scgms::SFilter_Configuration_Link link) {
			if (found) return;


			GUID id;
			found = link->Get_Filter_Id(&id) != S_OK;
			if (!found) {
				scgms::SFilter filter = create_filter(id, &terminal);
				found = !filter.operator bool();
				if (!found) {
					std::shared_ptr<scgms::IFilter_Feedback_Receiver> feedback_receiver;
					refcnt::Query_Interface<scgms::IFilter, scgms::IFilter_Feedback_Receiver>(filter.get(), scgms::IID_Filter_Feedback_Receiver, feedback_receiver);
					found = feedback_receiver.operator bool();
				}
			}			

			if (!found) minimal_index_end++;
		});

		return minimal_index_end;
	}


	bool Fetch_Events_To_Replay(refcnt::Swstr_list &error_description) {
		mEvents_To_Replay.clear();

		const size_t minimal_receiver_end = Find_Minimal_Receiver_Index_End();
		if (minimal_receiver_end == 0) return true;	//if it is the very first filter, than we can safely fetch no events to replay - but it is correct
													//or, we would need an additional logic to verify that no one connects to this filter
													//but that would be a special-case, strange behavior => not worth the effort

		mFirst_Effective_Filter_Index = std::min(*std::min_element(mFilter_Indices.begin(), mFilter_Indices.end()), minimal_receiver_end);
		scgms::SFilter_Chain_Configuration reduced_filter_configuration = Copy_Reduced_Configuration(mFirst_Effective_Filter_Index);


		std::recursive_mutex communication_guard;
		CCopying_Terminal_Filter terminal_filter{ mEvents_To_Replay };
		{
			CComposite_Filter composite_filter{ communication_guard };	//must be in the block that we can precisely
																		//call its dtor to get the future error properly
			if (composite_filter.Build_Filter_Chain(reduced_filter_configuration.get(), &terminal_filter, mOn_Filter_Created, mOn_Filter_Created_Data, error_description) == S_OK) {
				terminal_filter.Wait_For_Shutdown();
				return true;
			}
				else {
					composite_filter.Clear();	//terminate for sure
					mEvents_To_Replay.clear(); //sanitize as this might have been filled partially
					error_description.push(dsFailed_to_execute_first_filters);

					return false;
				}
		}
	}

	size_t Count_Objectives() {
		size_t result = 0;

		CNull_wstr_list empty_error_description{};
		TFast_Configuration clone = Clone_Configuration(mFirst_Effective_Filter_Index, empty_error_description);

		if (!clone.failed) {

			//Have the means to pickup the final metric
			CError_Metric_Future error_metric_future{ mOn_Filter_Created, mOn_Filter_Created_Data };

			//run the configuration
			std::recursive_mutex communication_guard;
			CTerminal_Filter terminal_filter{ nullptr };
			{
				CComposite_Filter composite_filter{ communication_guard };	//must be in the block that we can precisely
																			//call its dtor to get the future error properly

				if (composite_filter.Build_Filter_Chain(clone.configuration.get(), &terminal_filter, On_Filter_Created_Wrapper, &error_metric_future, empty_error_description) == S_OK) {
					//error metric future should hold the result now - just the count, the results! but we are intersted in the counts only
					std::array<double, solver::Maximum_Objectives_Count> tmp_metrics{ std::numeric_limits<double>::quiet_NaN() };
					result = error_metric_future.Get_Error_Metric(tmp_metrics.data());

					scgms::IDevice_Event* shutdown_event = allocate_device_event(scgms::NDevice_Event_Code::Shut_Down);
					if (Succeeded(composite_filter.Execute(shutdown_event)))
						terminal_filter.Wait_For_Shutdown();	//wait only if the shutdown did go through succesfully
				}

			}	//calls dtor of the signal error filter, thus filling the future error metric
				//but we cared about the count only
		}

		return result;
	}

protected:
	const scgms::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;
protected:
	scgms::SFilter_Chain_Configuration mConfiguration;
	const std::vector<size_t> mFilter_Indices;
	std::vector<size_t> mFilter_Parameter_Counts, mFilter_Parameter_Offsets;
	size_t mFirst_Effective_Filter_Index = 0;
	const std::vector<std::wstring> mParameters_Config_Names;

	std::mutex mClone_Guard;

	TFast_Configuration Clone_Configuration(const size_t first_effective_filter, refcnt::Swstr_list error_description) {
		std::lock_guard<std::mutex> lg{ mClone_Guard };

		TFast_Configuration result;
		result.configuration = Empty_Chain();
		result.first_parameter_ptrs.resize(mFilter_Indices.size());
		result.failed = !result.configuration.operator bool();

		const std::set<GUID> ui_filters = { scgms::IID_Drawing_Filter, scgms::IID_Log_Filter };	//let's optimize away those filters, which would only slow down

		//we do not need to do a complete copy -> we just need to 
		// 1. create the root configuration container, because the we can
		// 2. create a new link configuration for the given filter
		// 3. where, we insert new configuration-parameters
		//thus all other objects stays the same, we only increase their reference counters

		
		size_t link_counter = 0;
		if (!result.failed)
			mConfiguration.for_each([&link_counter, &result, &ui_filters, this, first_effective_filter, &error_description](scgms::SFilter_Configuration_Link src_link) {
				if (result.failed) return;

				if ((link_counter < first_effective_filter) && !mEvents_To_Replay.empty()) {
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

				scgms::IFilter_Configuration_Link* raw_link_to_add = src_link.get();

				for (size_t index = 0; index < mFilter_Indices.size(); index++) {

					if (link_counter == mFilter_Indices[index]) {
						if (filter_id == Invalid_GUID) {
							result.failed = true;
							return;
						}

						raw_link_to_add = static_cast<scgms::IFilter_Configuration_Link*>(new CFilter_Configuration_Link(filter_id)); //so far, zero RC which is correct right now because we do not call dtor here

						{
							refcnt::wstr_container* parent_path = nullptr;
							result.failed = mConfiguration->Get_Parent_Path(&parent_path) != S_OK;
                                                        if (!result.failed) {
                                                                const auto converted_path = refcnt::WChar_Container_To_WString(parent_path);
                                                                result.failed = raw_link_to_add->Set_Parent_Path(converted_path.c_str()) != S_OK;
                                                        }
							parent_path->Release();
						}


						//now, we need to emplace new configuration-parameters

						bool found_parameters = false;
						src_link.for_each([&raw_link_to_add, &found_parameters, &result, this, index](scgms::SFilter_Parameter src_parameter) {

							scgms::IFilter_Parameter* raw_parameter = src_parameter.get();

							if (!found_parameters && (mParameters_Config_Names[index] == src_parameter.configuration_name())) {
								//parameters - we need to create a new copy

								raw_parameter = static_cast<scgms::IFilter_Parameter*>(new CFilter_Parameter{ scgms::NParameter_Type::ptDouble_Array, mParameters_Config_Names[index].c_str() });
								scgms::IModel_Parameter_Vector *src_parameters, *dst_parameters;
								if (src_parameter->Get_Model_Parameters(&src_parameters) == S_OK) {
									dst_parameters = refcnt::Copy_Container<double>(src_parameters);
									if (raw_parameter->Set_Model_Parameters(dst_parameters) != S_OK)
										result.failed = true;	//increases RC by 1
									src_parameters->Release();
									dst_parameters->Release();

									//find the very first number to overwrite later on with new values
									double *begin, *end;
									if (dst_parameters->get(&begin, &end) == S_OK) {
										result.first_parameter_ptrs[index] = begin + mFilter_Parameter_Counts[index];
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

						if (!found_parameters) {
							result.failed = true;	//we need the parameters, because they also include the bounds!
							error_description.push(dsParameters_to_optimize_not_found);
						}

					}
				}

				result.configuration->add(&raw_link_to_add, &raw_link_to_add + 1);	//increases RC by one
				link_counter++;

			});
		
		if (result.failed) {
			error_description.push(dsFailed_to_clone_configuration);
		}

		return result;
	}

	
public:
	static inline CNull_wstr_list mEmpty_Error_Description;	//no thread local as we need to reset it!
public:
	CParameters_Optimizer(scgms::IFilter_Chain_Configuration *configuration, const size_t *filter_indices, const wchar_t **parameters_config_names, const size_t filter_count, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data)
		: mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data),
		mConfiguration(refcnt::make_shared_reference_ext<scgms::SFilter_Chain_Configuration, scgms::IFilter_Chain_Configuration>(configuration, true)),
		mFilter_Indices{ filter_indices, filter_indices + filter_count }, mParameters_Config_Names{ parameters_config_names, parameters_config_names + filter_count } {

		//mEmpty_Error_Description.reset();
		//having this with nullptr, no error write will actually occur
		//actually, many errors may arise due to the use of genetic algorithm -> let's suppress them
		//end user has the chance the debug the configuration first, by running a single isntance	

	}


	~CParameters_Optimizer() {
		//for (auto &event : mEvents_To_Replay)
//			event->Release();
	}

	HRESULT Optimize(const GUID solver_id, const size_t population_size, const size_t max_generations, 
		const double** hints, const size_t hint_count,
		solver::TSolver_Progress &progress, refcnt::Swstr_list error_description) {

		mLower_Bound.clear();
		mFound_Parameters.clear();
		mUpper_Bound.clear();

		mFilter_Parameter_Counts.resize(mFilter_Indices.size());
		mFilter_Parameter_Offsets.resize(mFilter_Indices.size());

		for (size_t i = 0; i < mFilter_Indices.size(); i++) {

			scgms::SFilter_Configuration_Link configuration_link_parameters = mConfiguration[mFilter_Indices[i]];

			if (!configuration_link_parameters) {
				error_description.push(dsParameters_to_optimize_not_found);
				return E_INVALIDARG;
			}

			std::vector<double> lbound, params, ubound;
			if (!configuration_link_parameters.Read_Parameters(mParameters_Config_Names[i].c_str(), lbound, params, ubound)) {
				error_description.push(dsParameters_to_optimize_could_not_be_read_bounds_including);
				return E_FAIL;
			}

			mFilter_Parameter_Offsets[i] = mFound_Parameters.size();	// where does the parameter set begin
			mFilter_Parameter_Counts[i] = params.size();				// how many parameters does this parameter set have

			std::copy(lbound.begin(), lbound.end(), std::back_inserter(mLower_Bound));
			std::copy(params.begin(), params.end(), std::back_inserter(mFound_Parameters));
			std::copy(ubound.begin(), ubound.end(), std::back_inserter(mUpper_Bound));
		}

		mProblem_Size = mFound_Parameters.size();

		//create initial pool-configuration to determine problem size
		//and to verify that we are able to clone the configuration
		{
			TFast_Configuration configuration = Clone_Configuration(0, error_description);
			if (configuration.failed)
				return E_FAIL;	//we release this configuration, because it has not used mFirst_Effective_Filter_Index
		}

		if (!Fetch_Events_To_Replay(error_description)) return E_FAIL;
		mObjectives_Count = Count_Objectives();
		if (mObjectives_Count == 0) {
			error_description.push(dsUnsupported_Metric_Configuration);
			return E_FAIL;
		}

		//const double* default_parameters = mFound_Parameters.data();

		std::vector<const double*> effective_hints;
		effective_hints.push_back(mFound_Parameters.data());
		for (size_t i = 0; i < hint_count; i++)
			effective_hints.push_back(hints[i]);

		solver::TSolver_Setup solver_setup{
			mProblem_Size, mObjectives_Count,
			mLower_Bound.data(), mUpper_Bound.data(),
			effective_hints.data(), effective_hints.size(),					//hints
			mFound_Parameters.data(),

			this, internal::Parameters_Fitness_Wrapper, nullptr,
			max_generations, population_size, std::numeric_limits<double>::min()
		};

		HRESULT rc = solve_generic(&solver_id, &solver_setup, &progress);

		//validate the returned fitness by re-computing the returned parameters
		if (rc == S_OK) {
			if (!Calculate_Fitness(1, mFound_Parameters.data(), progress.best_metric.data(), error_description))
				rc = E_UNEXPECTED;	//cannot compute the validation fitness
		}

		//eventually, we need to copy the parameters to the original configuration - on success
		if (rc == S_OK) {
			for (size_t i = 0; i < mFilter_Indices.size(); i++) {
				
				scgms::SFilter_Configuration_Link configuration_link_parameters = mConfiguration[mFilter_Indices[i]];
			
				std::vector<double> lbound, params, ubound;
				Get_Parameter_Subset(i, mLower_Bound, lbound);
				Get_Parameter_Subset(i, mFound_Parameters, params);
				Get_Parameter_Subset(i, mUpper_Bound, ubound);
				
				rc = configuration_link_parameters.Write_Parameters(mParameters_Config_Names[i].c_str(), lbound, params, ubound) ? rc : E_FAIL;
				
				if (rc != S_OK) {
					error_description.push(dsFailed_to_write_parameters);
					break;
				}
			}
		}
		else
			error_description.push(dsSolver_Failed);

		return rc;
	}

	void Get_Parameter_Subset(const size_t optimized_filter_idx, const std::vector<double>& parameterSet, std::vector<double>& target)
	{
		target.resize(mFilter_Parameter_Counts[optimized_filter_idx]);
		std::copy(parameterSet.begin() + mFilter_Parameter_Offsets[optimized_filter_idx], parameterSet.begin() + mFilter_Parameter_Offsets[optimized_filter_idx] + mFilter_Parameter_Counts[optimized_filter_idx], target.begin());
	}

	bool Calculate_Fitness(const size_t solution_count, const double* solutions, double* const fitnesses, refcnt::Swstr_list empty_error_description) {
		if (solution_count > 1) {
			bool success_flag = true;
			std::for_each(std::execution::par/* par_unseq*/, solver::CInt_Iterator<size_t>{ 0 }, solver::CInt_Iterator<size_t>{ solution_count }, [=, &success_flag](const auto& id) {			
				// we use execution::par only to avoid any possible problems with nested fitness calls, e.g. to OpenCL in the future
				if (success_flag) {
					if (!Calculate_Single_Fitness(solutions + id * mProblem_Size, fitnesses + id * solver::Maximum_Objectives_Count, empty_error_description))
						success_flag = false;	//no need for locking because any thread may write the same value
				}
			});

			return success_flag;
		}
		else
			return Calculate_Single_Fitness(solutions, fitnesses, empty_error_description);
	}


	bool Calculate_Single_Fitness(const double* solution, double* const fitness, refcnt::Swstr_list empty_error_description) {
		bool failure_detected = false;

		TFast_Configuration configuration = Clone_Configuration(mFirst_Effective_Filter_Index, empty_error_description);	//later on, we will replace this with a pool
																								//or rather not as there might a be problem with filters,
																								//which would keep, but not reset their states		

		//set the experimental parameters
		for (size_t i = 0; i < configuration.first_parameter_ptrs.size(); i++) {
			const double* base = reinterpret_cast<const double*>(solution) + mFilter_Parameter_Offsets[i];
			std::copy(base, base + mFilter_Parameter_Counts[i], configuration.first_parameter_ptrs[i]);
		}

		//Have the means to pickup the final metric
		CError_Metric_Future error_metric_future{ mOn_Filter_Created, mOn_Filter_Created_Data };
		
		//run the configuration
		std::recursive_mutex communication_guard;
		CTerminal_Filter terminal_filter{ nullptr };
		{			
			CComposite_Filter composite_filter{ communication_guard };	//must be in the block that we can precisely
																		//call its dtor to get the future error properly

			if (composite_filter.Build_Filter_Chain(configuration.configuration.get(), &terminal_filter, On_Filter_Created_Wrapper, &error_metric_future, empty_error_description) != S_OK)
				return std::numeric_limits<double>::quiet_NaN();
			
			//wait for the result
			if (!mEvents_To_Replay.empty()) {
				for (size_t i = 0; i < mEvents_To_Replay.size(); i++) {		//we can replay the pre-calculated events					

					scgms::IDevice_Event* event_to_replay = nullptr;
					failure_detected = !Succeeded(mEvents_To_Replay[i].Clone(&event_to_replay));
					
					if (!failure_detected) {
						//std::lock_guard<std::mutex> lg{ mClone_Guard };	//TODO: this possibly masks some another bug
						failure_detected = !Succeeded(composite_filter.Execute(event_to_replay));
					}

					if (failure_detected) {
						//something has not gone well => break, but be that nice to issue the shutdown event first
						scgms::IDevice_Event* shutdown_event = allocate_device_event(scgms::NDevice_Event_Code::Shut_Down );
						if (Succeeded(composite_filter.Execute(shutdown_event)))
							terminal_filter.Wait_For_Shutdown();	//wait only if the shutdown did go through succesfully
						break;
					}
				}
			}
			else
				terminal_filter.Wait_For_Shutdown();//no pre-calculated events can be replayed=> we need to go the old-fashioned way

		}	//calls dtor of the signal error filter, thus filling the future error metric

		//once implemented, we should return the configuration back to the pool

		//pickup the fitnesses value/error metrics and return it		
		return (!failure_detected) && (error_metric_future.Get_Error_Metric(fitness) == mObjectives_Count);	//we have to pick at as many metrics as promised
	}
};



BOOL IfaceCalling internal::Parameters_Fitness_Wrapper(const void *data, const size_t solution_count, const double *solutions, double* const fitnesses) {
	CParameters_Optimizer *candidate = reinterpret_cast<CParameters_Optimizer*>(const_cast<void*>(data));
	return candidate->Calculate_Fitness(solution_count, solutions, fitnesses, CParameters_Optimizer::mEmpty_Error_Description) ? TRUE : FALSE;
}


HRESULT IfaceCalling optimize_parameters(scgms::IFilter_Chain_Configuration *configuration, const size_t filter_index, const wchar_t *parameters_configuration_name, 
										 scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
									     const GUID *solver_id, const size_t population_size, const size_t max_generations, 
										 const double** hints, const size_t hint_count,
										 solver::TSolver_Progress *progress,
										 refcnt::wstr_list *error_description) {

	CParameters_Optimizer optimizer{ configuration, &filter_index, &parameters_configuration_name, 1, on_filter_created, on_filter_created_data };
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);
	return optimizer.Optimize(*solver_id, population_size, max_generations, hints, hint_count, *progress, shared_error_description);
}

HRESULT IfaceCalling optimize_multiple_parameters(scgms::IFilter_Chain_Configuration *configuration, const size_t *filter_indices, const wchar_t **parameters_configuration_names, size_t filter_count,
												  scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
												  const GUID *solver_id, const size_t population_size, const size_t max_generations, 
												  const double** hints, const size_t hint_count,
												  solver::TSolver_Progress *progress,
												  refcnt::wstr_list *error_description) {

	CParameters_Optimizer optimizer{ configuration, filter_indices, parameters_configuration_names, filter_count, on_filter_created, on_filter_created_data };
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);
	return optimizer.Optimize(*solver_id, population_size, max_generations, hints, hint_count, *progress, shared_error_description);
}
