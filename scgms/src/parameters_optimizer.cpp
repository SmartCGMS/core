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

#include <scgms/rtl/SolverLib.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/utils/DebugHelper.h>
#include <scgms/lang/dstrings.h>

#include "parameters_optimizer.h"
#include "configuration_link.h"
#include "filter_parameter.h"
#include "filters.h"
#include "executor.h"
#include "composite_filter.h"
#include "device_event.h"
#include "persistent_chain_configuration.h"

#include <stack>
#include <mutex>
#include <set>
#include <numeric>

class CNull_wstr_list : public refcnt::Swstr_list {
		//hides all the methods to do nothing
		//i.e.; to even save the overhead of checking the null pointer as Swstr_list does
public:
	CNull_wstr_list() : SReferenced<refcnt::wstr_list>{ } {};
	void push(const wchar_t* wstr) {};
	void push(const std::wstring& wstr) {};

	void for_each(std::function<void(const std::wstring& wstr)> callback) const {};
};

struct TOptimizing_Configuration {
	scgms::SFilter_Chain_Configuration optimizing_body;
	std::vector<CDevice_Event> events_to_replay;
};

struct TConfig_Characteristics {
	size_t optimizing_body_begin = std::numeric_limits<size_t>::max();
	size_t optimizing_body_end = 0;
	size_t objective_count = 0;

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
	TConfig_Characteristics mCharacteristics;
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
	std::mutex mOptimizing_Pool_Guard;
	std::vector<CDevice_Event> mEvents_To_Replay_Master_Copy;
	scgms::SFilter_Chain_Configuration mOptimizing_Body_Master_Copy;

	std::stack<TOptimizing_Configuration> mOptimizing_Pool;
	
	TOptimizing_Configuration Pop_Optimizing_Configuration() {
		std::lock_guard<std::mutex> lg{ mOptimizing_Pool_Guard };
			//Note that modern mutexes already do have spinlocks, so to speed up, we would need to implement a TBB concurrent-queue alike with respecting cache line sizes.
			//However, this would be a significant effort and adding TBB would harm our portability. Now, none of it seems to be a cost-effective benefit.


		if (mOptimizing_Pool.empty()) {
			//it is empty, we need to construct a new one (which still needs the lock as it accesses the master copies)
			TOptimizing_Configuration result;

			//copy the events
			for (size_t i = 0; i < mEvents_To_Replay_Master_Copy.size(); i++) {
				scgms::TDevice_Event& src_event = mEvents_To_Replay_Master_Copy[i].Raw();
				CDevice_Event dst_event;
				dst_event.Initialize(&src_event);

				result.events_to_replay.push_back(std::move(dst_event));
			}


			//copy the config
			result.optimizing_body = Deep_Copy_Subconfiguration(0, mCharacteristics.optimizing_body_end - mCharacteristics.optimizing_body_begin, mOptimizing_Body_Master_Copy, false);

			return result;
		}
		else {
			//just pop the first available config and return it
			auto result = std::move(mOptimizing_Pool.top());
			mOptimizing_Pool.pop();
			return result;
		}
	}

	TOptimizing_Configuration Pop_Optimizing_Configuration(const double* solution) {
		TOptimizing_Configuration result = Pop_Optimizing_Configuration();	//now, we have unpatched clone, but we need to apply custom solution

		for (size_t i = 0; i < mFilter_Indices.size(); i++) {
			auto link = result.optimizing_body[mFilter_Indices[i] - mCharacteristics.optimizing_body_begin];
			auto param = link.Resolve_Parameter(mParameters_Config_Names[i].c_str());

			
			scgms::IModel_Parameter_Vector* params;
			bool failed = true;
			if (param->Get_Model_Parameters(&params) == S_OK) {
				double* begin, * end;
				if (params->get(&begin, &end) == S_OK) {
					failed = false;


					const double* base = reinterpret_cast<const double*>(solution) + mFilter_Parameter_Offsets[i];
					std::copy(base, base + mFilter_Parameter_Counts[i], begin + mFilter_Parameter_Counts[i]);	//we add the counts to skip lower bounds
				}

				params->Release();

			}

			if (failed) {
				result.optimizing_body.reset();
				return result;	//we are returning invalid configuration, that won't be pushed back
			}
		}

		return result;
	}

	void Push_Optimizing_Pool(TOptimizing_Configuration& config) {
		if (config.optimizing_body) {
				//push valid configs only
			std::lock_guard<std::mutex> lg{ mOptimizing_Pool_Guard };
			mOptimizing_Pool.push(std::move(config));
		}
	}

protected:
	template <typename T, typename L, typename S>
	HRESULT Remove_Scalar_Variable(scgms::IFilter_Parameter *param, L load, S store) {
		T val;
		HRESULT rc = (*param.*load)(&val);
		if (Succeeded(rc))
			rc = (*param.*store)(val);
		else
			if (rc == E_NOT_SET)
				rc = S_FALSE;
		return rc;
	}

	template <typename I, typename L, typename S>
	HRESULT Remove_Container_Variable(scgms::IFilter_Parameter* param, L load, S store) {
		I *cont = nullptr;
		HRESULT rc = (*param.*load)(&cont);
		if (rc == E_NOT_SET) {
			rc = S_FALSE;
			cont = nullptr;
		}

		if (Succeeded(rc)) {
			rc = (*param.*store)(cont);
			if (cont) 
				cont->Release();
		} 
		return rc;
	}

	bool Remove_Variables_From_Parameter(scgms::IFilter_Parameter* param) {
		scgms::NParameter_Type type = scgms::NParameter_Type::ptNull;
		HRESULT rc = param->Get_Type(&type);

		if (rc == S_OK) {
			switch (type) {
				case scgms::NParameter_Type::ptWChar_Array:
				{
					refcnt::wstr_container* wc = nullptr;
					rc = param->Get_WChar_Container(&wc, TRUE);
					if (rc == E_NOT_SET) {
						rc = S_FALSE;
						wc = nullptr;
					}

					if (Succeeded(rc)) {
						param->Set_WChar_Container(wc);
						if (wc)
							wc->Release();
					} 
				}
				break;

				case scgms::NParameter_Type::ptDouble_Array:
					rc = Remove_Container_Variable<scgms::IModel_Parameter_Vector>(param, &scgms::IFilter_Parameter::Get_Model_Parameters, &scgms::IFilter_Parameter::Set_Model_Parameters);
					break;

				case scgms::NParameter_Type::ptInt64_Array:
					rc = Remove_Container_Variable<scgms::time_segment_id_container>(param, &scgms::IFilter_Parameter::Get_Time_Segment_Id_Container, &scgms::IFilter_Parameter::Set_Time_Segment_Id_Container);
					break;

				case scgms::NParameter_Type::ptRatTime:
				case scgms::NParameter_Type::ptDouble:
					rc = Remove_Scalar_Variable<double>(param, &scgms::IFilter_Parameter::Get_Double, &scgms::IFilter_Parameter::Set_Double);
					break;

				case scgms::NParameter_Type::ptInt64:
				case scgms::NParameter_Type::ptSubject_Id:
					rc = Remove_Scalar_Variable<int64_t>(param, &scgms::IFilter_Parameter::Get_Int64, &scgms::IFilter_Parameter::Set_Int64);
				break;

				case scgms::NParameter_Type::ptBool:
					rc = Remove_Scalar_Variable<BOOL>(param, &scgms::IFilter_Parameter::Get_Bool, &scgms::IFilter_Parameter::Set_Bool);
					break;

				case scgms::NParameter_Type::ptSignal_Model_Id:
				case scgms::NParameter_Type::ptDiscrete_Model_Id:
				case scgms::NParameter_Type::ptMetric_Id:
				case scgms::NParameter_Type::ptModel_Produced_Signal_Id:
				case scgms::NParameter_Type::ptSignal_Id:
				case scgms::NParameter_Type::ptSolver_Id:
				{
					GUID id;
					rc = param->Get_GUID(&id);
					if (rc == E_NOT_SET) {
						id = Invalid_GUID;
						rc = S_FALSE;
					}

					if (Succeeded(rc))
						rc = param->Set_GUID(&id);
				}
				break;

				default: break;
			} //switch (source_type) {
		}

		return Succeeded(rc);
	}


	scgms::SFilter_Chain_Configuration Deep_Copy_Subconfiguration(const size_t begin_index, const size_t end_index, scgms::SFilter_Chain_Configuration src_config, const bool remove_variables) {
		//note that scgms filter is permitted to update its configuration => hence, we need to do deep copy so that such possible changes stay isoloated within each candidate solution
		//currently, it does not seem effective to introduce an additional flag for such filters, while adding more code to spare some object instantiations by using RC-shared read-only config links and parameters
		
		scgms::SFilter_Chain_Configuration reduced_filter_configuration = Empty_Chain();
		bool success = reduced_filter_configuration.operator bool();

		if (success) {
			refcnt::wstr_container* parent_path = nullptr;
			success = src_config->Get_Parent_Path(&parent_path) == S_OK;
			if (success) {
				const auto converted_path = refcnt::WChar_Container_To_WString(parent_path);
				success = reduced_filter_configuration->Set_Parent_Path(converted_path.c_str()) == S_OK;
				parent_path->Release();
			}


			for (size_t link_counter = 0; (link_counter < end_index) && success; link_counter++) {
				if (link_counter >= begin_index) {
					scgms::SFilter_Configuration_Link src_link = src_config[link_counter];
					if (!src_link) {
						success = false;
						break;
					}
					std::unique_ptr<CFilter_Configuration_Link> dst_link = std::make_unique<CFilter_Configuration_Link>(src_link.descriptor().id);

					//let's clone the solution
					src_link.for_each([&dst_link, &success, remove_variables, this](scgms::SFilter_Parameter src_parameter) {
						if (success) {
							scgms::IFilter_Parameter* deep_copy = nullptr;
							success = src_parameter->Clone(&deep_copy) == S_OK;

							if (success && remove_variables)
								success = Remove_Variables_From_Parameter(deep_copy);

							if (success) {
								success = dst_link->add(&deep_copy, &deep_copy + 1) == S_OK;
								if (!success)
									deep_copy->Release();
							}
						}
					});

					if (success) {
						scgms::IFilter_Configuration_Link* raw_dst_link = dst_link.get();
						dst_link.release();
						success = reduced_filter_configuration->add(&raw_dst_link, &raw_dst_link + 1) == S_OK;
						if (!success)
							raw_dst_link->Release();
					}
				}
			}
		}

		return success ? reduced_filter_configuration : Empty_Chain();
	}

		//The configuration can be split to three parts
		//  a) head part, which only generates events regardless of the solution, which are trying to optimize => needs to be instantiated just once to fetch the const source of events
		//  b) optimizing part, which processes events produced from the previous, head-part according to the solution we are trying to optimize - this part produces metrics and may contain feedbacks
		//		=> this part must be instantiated for all candidate paramterws => we wish it is as small as possible
		//	c) tail part, which produces no metric nor realizes any feedback => does not need to be instantiated at all
		// 
		// Hence, the function returns first and last+1 indicies of the body part.
		//Note that with a special, all-doing filter (despite being this a bad design) {0, 0} is a valid return value
	std::tuple<HRESULT, TConfig_Characteristics> Count_Config_Characteristics(refcnt::Swstr_list& error_description) {
		size_t first_feedback_receiver_idx = std::numeric_limits<size_t>::max();
		size_t last_metric_or_feedback_sender_idx = 0;
		bool ok = true;
		size_t filter_counter = 0;
		size_t objective_count = 0;
		
		CTerminal_Filter terminal{ nullptr };

		mConfiguration.for_each([&](scgms::SFilter_Configuration_Link link) {
			//we need to traverse entire configuration to find both indices
			if (!ok)
				return;

			GUID id;
			ok = link->Get_Filter_Id(&id) == S_OK;
			if (ok) {
				scgms::SFilter filter = create_filter_body(id, &terminal);
				ok = filter.operator bool();
				if (ok) {
					if (first_feedback_receiver_idx == std::numeric_limits<size_t>::max()) {
						//try feedback receiver, if have not already found it
						std::shared_ptr<scgms::IFilter_Feedback_Receiver> feedback_receiver;
						refcnt::Query_Interface<scgms::IFilter, scgms::IFilter_Feedback_Receiver>(filter.get(), scgms::IID_Filter_Feedback_Receiver, feedback_receiver);
						if (feedback_receiver.operator bool())
							first_feedback_receiver_idx = last_metric_or_feedback_sender_idx = filter_counter;
						//we do both assignments to enforce valid ordering of the config parts
					}

					{
						//let's check metric and feedback sender
						//err insp goes first as we need to count the metrics
						scgms::SSignal_Error_Inspection error_insp;
						refcnt::Query_Interface<scgms::IFilter, scgms::ISignal_Error_Inspection>(filter.get(), scgms::IID_Signal_Error_Inspection, error_insp);
						if (error_insp.operator bool()) {
							last_metric_or_feedback_sender_idx = filter_counter;
							objective_count++;
						}
						else {	//we do else, because one of these ifaces is enough
						 //do not forget to try a feedback sender iface too
							std::shared_ptr<scgms::IFilter_Feedback_Sender> feedback_sender;
							refcnt::Query_Interface<scgms::IFilter, scgms::IFilter_Feedback_Sender>(filter.get(), scgms::IID_Filter_Feedback_Sender, feedback_sender);
							if (feedback_sender.operator bool()) {
								last_metric_or_feedback_sender_idx = filter_counter;
							}
						}

					}

				}
				else 
					error_description.push(rsFailed_to_allocate_memory);
			} else
				error_description.push(dsCannot_Resolve_Filter_Descriptor);
			

			filter_counter++;
		});
		
		
		//last metric or feedback sender idx  is the optimizing_body_end
		//=> we need to designate the optimizing body begin
		if (first_feedback_receiver_idx == std::numeric_limits<size_t>::max()) {
			//there is no feedback receiver
			first_feedback_receiver_idx = last_metric_or_feedback_sender_idx;
		}
		
		//the body part must start at least with the first filter whose solution we are about to optimize
		first_feedback_receiver_idx = std::min(first_feedback_receiver_idx, mFilter_Indices[0]);


		return { ok ? S_OK : E_FAIL, {first_feedback_receiver_idx, last_metric_or_feedback_sender_idx+1, objective_count} };	//+1 is like the end iterator!
	}


	bool Fetch_Events_To_Replay(const size_t optimizing_body_begin, refcnt::Swstr_list &error_description) {
		mEvents_To_Replay_Master_Copy.clear();
		 
		if (optimizing_body_begin == 0) return true;	//if it is the very first filter, than we can safely fetch no events to replay - but it is correct
													//or, we would need an additional logic to verify that no one connects to this filter
													//but that would be a special-case, strange behavior => not worth the effort

		scgms::SFilter_Chain_Configuration reduced_filter_configuration = Deep_Copy_Subconfiguration(0, optimizing_body_begin, mConfiguration, false);


		std::recursive_mutex communication_guard;
		CCopying_Terminal_Filter terminal_filter{ mEvents_To_Replay_Master_Copy, true };
					//when copying, we avoid any info events as:
					//	1 - it is a bad practice to controle anything with them
					//  2 - we have no use for any text info, when optimizing
		{
			CComposite_Filter composite_filter{ communication_guard };	//must be in the block that we can precisely
																		//call its dtor to get the future error properly
			if (composite_filter.Build_Filter_Chain(reduced_filter_configuration.get(), &terminal_filter, mOn_Filter_Created, mOn_Filter_Created_Data, error_description) == S_OK) {
				terminal_filter.Wait_For_Shutdown();
				return true;
			}
				else {
					composite_filter.Clear();	//terminate for sure
					mEvents_To_Replay_Master_Copy.clear(); //sanitize as this might have been filled partially
					error_description.push(dsFailed_to_execute_first_filters);

					return false;
				}
		}
	}


protected:
	const scgms::TOn_Filter_Created mOn_Filter_Created;
	const void* mOn_Filter_Created_Data;
protected:
	scgms::SFilter_Chain_Configuration mConfiguration;
	std::vector<size_t> mFilter_Indices;
	std::vector<size_t> mFilter_Parameter_Counts, mFilter_Parameter_Offsets;
	size_t mFirst_Effective_Filter_Index = 0;
	std::vector<std::wstring> mParameters_Config_Names;

		HRESULT Prepare_Parameters(refcnt::Swstr_list error_description) {
		if (mFilter_Indices.size() < 1) {
			error_description.push(dsParameters_to_optimize_not_found);
			error_description.push(L"You must give non-zero number of parameters to optimize.");
			return E_INVALIDARG;
		}


		if (mFilter_Indices.size() > 1) {
			std::vector<size_t> remap_indices(mFilter_Indices.size());
			std::iota(remap_indices.begin(), remap_indices.end(), 0);
			std::sort(remap_indices.begin(), remap_indices.end(), [this](const size_t a, const size_t b) {
				return mFilter_Indices[a] < mFilter_Indices[b];
			});


			for (size_t i = 0; i < remap_indices.size(); i++) {
				while (remap_indices[i] != i) {
					std::swap(mFilter_Indices[i], mFilter_Indices[remap_indices[i]]);
					std::swap(mParameters_Config_Names[i], mParameters_Config_Names[remap_indices[i]]);
					std::swap(remap_indices[i], remap_indices[remap_indices[i]]);
				}
			}
		}

		mLower_Bound.clear();
		mFound_Parameters.clear();
		mUpper_Bound.clear();

		mFilter_Parameter_Counts.clear();
		mFilter_Parameter_Offsets.clear();

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

			mFilter_Parameter_Offsets.push_back(mFound_Parameters.size());	// where does the parameter set begin
			mFilter_Parameter_Counts.push_back(params.size());				// how many solution does this parameter set have

			std::copy(lbound.begin(), lbound.end(), std::back_inserter(mLower_Bound));
			std::copy(params.begin(), params.end(), std::back_inserter(mFound_Parameters));
			std::copy(ubound.begin(), ubound.end(), std::back_inserter(mUpper_Bound));
		}

		mProblem_Size = mFound_Parameters.size();

		return S_OK;
	}
public:
	static inline CNull_wstr_list mEmpty_Error_Description;	//no thread local as we need to reset it!
public:
	CParameters_Optimizer(scgms::IFilter_Chain_Configuration *configuration, const size_t *filter_indices, const wchar_t **parameters_config_names, const size_t filter_count, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data)
		: mOn_Filter_Created(on_filter_created), mOn_Filter_Created_Data(on_filter_created_data),
		mConfiguration(refcnt::Manufacture_Object_Shared<CPersistent_Chain_Configuration, scgms::IFilter_Chain_Configuration, scgms::SFilter_Chain_Configuration>()),
		mFilter_Indices{ filter_indices, filter_indices + filter_count }, mParameters_Config_Names{ parameters_config_names, parameters_config_names + filter_count } {


		//strip presentation-only filters from the configuration
		auto src_chain = refcnt::make_shared_reference_ext<scgms::SFilter_Chain_Configuration, scgms::IFilter_Chain_Configuration>(configuration, true);
		src_chain.for_each([this](scgms::SFilter_Configuration_Link src_link) {
			//let us add only those links, which are not presetnation only
			if ((src_link.descriptor().flags & scgms::NFilter_Flags::Presentation_Only) == scgms::NFilter_Flags::None) {
				scgms::IFilter_Configuration_Link* raw_link = src_link.get();
				mConfiguration->add(&raw_link, &raw_link + 1);
			}
		});


		refcnt::wstr_container* parent_path = nullptr;
		if (Succeeded(src_chain->Get_Parent_Path(&parent_path))) {
			const auto converted_path = refcnt::WChar_Container_To_WString(parent_path);
			if (!Succeeded(mConfiguration->Set_Parent_Path(converted_path.c_str())))
				mConfiguration.reset();
			parent_path->Release();
		}
		else
			mConfiguration.reset();

		//mEmpty_Error_Description.reset();
		//having this with nullptr, no error write will actually occur
		//actually, many errors may arise due to the use of genetic algorithm -> let's suppress them
		//end user has the chance the debug the configuration first, by running a single isntance	

	}


	~CParameters_Optimizer() {
		//for (auto &event : mEvents_To_Replay_Master_Copy)
//			event->Release();
	}

	

	HRESULT Optimize(const GUID solver_id, const size_t population_size, const size_t max_generations, 
		const double** hints, const size_t hint_count,
		solver::TSolver_Progress &progress, refcnt::Swstr_list error_description) {

		
		HRESULT rc = Prepare_Parameters(error_description);
		if (rc != S_OK)
			return rc;

		std::tie<HRESULT, TConfig_Characteristics>(rc, mCharacteristics) = Count_Config_Characteristics(error_description);
		//let's check the characteristics
		if (rc != S_OK)
			return rc;
		


		if ((mCharacteristics.objective_count == 0) || (mCharacteristics.objective_count > solver::Maximum_Objectives_Count)) {
			error_description.push(dsUnsupported_Metric_Configuration);
			return E_FAIL;		
		}

		if (!Fetch_Events_To_Replay(mCharacteristics.optimizing_body_begin, error_description))
			return E_FAIL;

		//now, we have  mEvents_To_Replay_Master_Copy filled, and have to fill optimizing body's master copy	
		//this is the only time we request to eliminate any variables in the configuration so that each candidate solution will have the possibly-used variables resolve to exactly the same values
		mOptimizing_Body_Master_Copy = Deep_Copy_Subconfiguration(mCharacteristics.optimizing_body_begin, mCharacteristics.optimizing_body_end, mConfiguration, true);
		if (!mOptimizing_Body_Master_Copy) {
			error_description.push(dsFailed_to_clone_configuration);
			return E_FAIL;
		}

	

		//const double* default_parameters = mFound_Parameters.data();

		std::vector<const double*> effective_hints;
		effective_hints.push_back(mFound_Parameters.data());
		for (size_t i = 0; i < hint_count; i++)
			effective_hints.push_back(hints[i]);

		solver::TSolver_Setup solver_setup{
			mProblem_Size, mCharacteristics.objective_count,
			mLower_Bound.data(), mUpper_Bound.data(),
			effective_hints.data(), effective_hints.size(),					//hints
			mFound_Parameters.data(),

			this, internal::Parameters_Fitness_Wrapper, nullptr,
			max_generations, population_size, std::numeric_limits<double>::min()
		};


		//as the configuration may hold some native filters, let us keep an over-subscription reduced instance to keep the libraries loaded
		//run the configuration		

		{
			std::recursive_mutex oversubscript_communication_guard;
			CTerminal_Filter oversubscript_terminal_filter{ nullptr };
			{
				TOptimizing_Configuration oversubscript_configuration = Pop_Optimizing_Configuration();
				CComposite_Filter oversubscript_composite_filter{ oversubscript_communication_guard };
				if (oversubscript_composite_filter.Build_Filter_Chain(oversubscript_configuration.optimizing_body.get(), &oversubscript_terminal_filter, nullptr, nullptr, error_description) != S_OK) {
					error_description.push(L"Cannot create the oversubscription configuration!");
					return E_FAIL;
				}

				//optimizing now...
				rc = solve_generic(&solver_id, &solver_setup, &progress);

				scgms::IDevice_Event* oversubscript_shutdown_event = allocate_device_event(scgms::NDevice_Event_Code::Shut_Down);
				if (Succeeded(oversubscript_composite_filter.Execute(oversubscript_shutdown_event)))
					oversubscript_terminal_filter.Wait_For_Shutdown();	//wait only if the shutdown did go through succesfully
			}
		}

		//validate the returned fitness by re-computing the returned solution
		if (rc == S_OK) {
			if (!Calculate_Fitness(1, mFound_Parameters.data(), progress.best_metric.data(), error_description))
				rc = E_UNEXPECTED;	//cannot compute the validation fitness
		}

		//eventually, we need to copy the solution to the original configuration - on success
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

		TOptimizing_Configuration opt = Pop_Optimizing_Configuration(solution);
		if (!opt.optimizing_body)
			return false;


		//Have the means to pickup the final metric
		CError_Metric_Future error_metric_future{ mOn_Filter_Created, mOn_Filter_Created_Data };

		//run the configuration
		std::recursive_mutex communication_guard;
		CTerminal_Filter terminal_filter{ nullptr };
		{
			CComposite_Filter composite_filter{ communication_guard };	//must be in the block that we can precisely
																		//call its dtor to get the future error properly

			if (composite_filter.Build_Filter_Chain(opt.optimizing_body.get(), &terminal_filter, On_Filter_Created_Wrapper, &error_metric_future, empty_error_description) != S_OK)
					return std::numeric_limits<double>::quiet_NaN();

			//wait for the result
			if (!opt.events_to_replay.empty()) {
				for (size_t i = 0; i < opt.events_to_replay.size(); i++) {		//we can replay the pre-calculated events

					scgms::IDevice_Event* event_to_replay = nullptr;
					failure_detected = !Succeeded(opt.events_to_replay[i].Clone(&event_to_replay));

					const auto event_code = opt.events_to_replay[i].Raw().event_code;
					if ((event_code == scgms::NDevice_Event_Code::Parameters) || (event_code == scgms::NDevice_Event_Code::Parameters_Hint)) {
						//such event carries a reference to parameters array, which is shared across the optimizing pool
						//to preserve the original value, let us make a deep copy of it in case that some filter would modify it - which would advertly affect entire optimization


						scgms::TDevice_Event* raw_event_to_replay = nullptr;
						failure_detected = !Succeeded(event_to_replay->Raw(&raw_event_to_replay));

						if (!failure_detected) {
							if (raw_event_to_replay->parameters) {
								scgms::IModel_Parameter_Vector* new_vector = refcnt::Copy_Container<double, scgms::IModel_Parameter_Vector>(raw_event_to_replay->parameters);
								raw_event_to_replay->parameters->Release();	//copied
								raw_event_to_replay->parameters = new_vector;	//and replaced with their deep copy
							}
						}

						//note we do not do the same for strings, because we already removed any string events from being replayed - see Fetch_Events_To_Replay
					}

					if (!failure_detected) {
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

		//return the configuration back to the pool
		Push_Optimizing_Pool(opt);

		//pickup the fitnesses value/error metrics and return it
		return (!failure_detected) && (error_metric_future.Get_Error_Metric(fitness) == mCharacteristics.objective_count);	//we have to pick at as many metrics as promised
	}
};



BOOL IfaceCalling internal::Parameters_Fitness_Wrapper(const void *data, const size_t solution_count, const double *solutions, double* const fitnesses) {
	CParameters_Optimizer *candidate = reinterpret_cast<CParameters_Optimizer*>(const_cast<void*>(data));
	return candidate->Calculate_Fitness(solution_count, solutions, fitnesses, CParameters_Optimizer::mEmpty_Error_Description) ? TRUE : FALSE;
}


DLL_EXPORT HRESULT IfaceCalling optimize_parameters(scgms::IFilter_Chain_Configuration *configuration, const size_t filter_index, const wchar_t *parameters_configuration_name,
										 scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
									     const GUID *solver_id, const size_t population_size, const size_t max_generations, 
										 const double** hints, const size_t hint_count,
										 solver::TSolver_Progress *progress,
										 refcnt::wstr_list *error_description) {

	CParameters_Optimizer optimizer{ configuration, &filter_index, &parameters_configuration_name, 1, on_filter_created, on_filter_created_data };
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);
	return optimizer.Optimize(*solver_id, population_size, max_generations, hints, hint_count, *progress, shared_error_description);
}

DLL_EXPORT HRESULT IfaceCalling optimize_multiple_parameters(scgms::IFilter_Chain_Configuration *configuration, const size_t *filter_indices, const wchar_t **parameters_configuration_names, size_t filter_count,
												  scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
												  const GUID *solver_id, const size_t population_size, const size_t max_generations, 
												  const double** hints, const size_t hint_count,
												  solver::TSolver_Progress *progress,
												  refcnt::wstr_list *error_description) {


	CParameters_Optimizer optimizer{ configuration, filter_indices, parameters_configuration_names, filter_count, on_filter_created, on_filter_created_data };
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);
	return optimizer.Optimize(*solver_id, population_size, max_generations, hints, hint_count, *progress, shared_error_description);
}
