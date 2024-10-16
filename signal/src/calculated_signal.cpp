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

#include "calculated_signal.h"

#include "descriptor.h"
#include "fitness.h"

#include <scgms/rtl/UILib.h>
#include <scgms/rtl/rattime.h>
#include <scgms/lang/dstrings.h>
#include <scgms/utils/math_utils.h>
#include <scgms/utils/string_utils.h>

#include <iostream>
#include <cmath>

constexpr unsigned char bool_2_uc(const bool b) {
	return b ? static_cast<unsigned char>(1) : static_cast<unsigned char>(0);
}

CCalculate_Filter::CCalculate_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

CCalculate_Filter::~CCalculate_Filter() {
	mSolver_Progress.cancelled = TRUE;
}

HRESULT IfaceCalling CCalculate_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<scgms::ICalculate_Filter_Inspection>(scgms::IID_Calculate_Filter_Inspection, *riid, ppvObj)) {
		return S_OK;
	}

	return E_NOINTERFACE;
}

std::unique_ptr<CTime_Segment>& CCalculate_Filter::Get_Segment(const uint64_t segment_id) {
	const auto iter = mSegments.find(segment_id);

	if (iter != mSegments.end()) {
		return iter->second;
	}
	else {
		std::unique_ptr<CTime_Segment> segment = std::make_unique<CTime_Segment>(segment_id, mCalculated_Signal_Id, mDefault_Parameters, mPrediction_Window, mOutput);
		const auto ret = mSegments.insert(std::make_pair(segment_id, std::move(segment)));
		return ret.first->second;
	}
}

HRESULT IfaceCalling CCalculate_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mCalculated_Signal_Id = configuration.Read_GUID(rsSelected_Signal);
	mPrediction_Window = configuration.Read_Double(rsPrediction_Window);
	mSolver_Enabled = configuration.Read_Bool(rsSolve_Parameters);
	mSolve_On_Calibration = configuration.Read_Bool(rsSolve_On_Calibration);
	mSolve_On_Time_Segment_End = configuration.Read_Bool(rsSolve_On_Time_Segment_End);
	mSolve_All_Segments = configuration.Read_Bool(rsSolve_Using_All_Segments);
	mReference_Level_Threshold_Count = configuration.Read_Int(rsSolve_On_Level_Count);
	mSolving_Scheduled = false;
	mReference_Level_Counter = 0;

	mSolver_Status = mSolver_Enabled ? scgms::TSolver_Status::Idle : scgms::TSolver_Status::Disabled;

	mSolver_Id = configuration.Read_GUID(rsSelected_Solver);

	//get reference signal id
	scgms::TModel_Descriptor desc = scgms::Null_Model_Descriptor;
	if (scgms::get_model_descriptor_by_signal_id(mCalculated_Signal_Id, desc)) {

		std::vector<double> lower_bound, default_parameters, upper_bound;

		if (!configuration.Read_Parameters(rsSelected_Model_Bounds, lower_bound, default_parameters, upper_bound)) {
			lower_bound.assign(desc.lower_bound, desc.lower_bound + desc.total_number_of_parameters);
			default_parameters.assign(desc.default_values, desc.default_values + desc.total_number_of_parameters);
			upper_bound.assign(desc.upper_bound, desc.upper_bound + desc.total_number_of_parameters);
		}

		mLower_Bound = refcnt::Create_Container_shared<double, scgms::SModel_Parameter_Vector>(lower_bound.data(), lower_bound.data() + lower_bound.size());
		mDefault_Parameters = refcnt::Create_Container_shared<double, scgms::SModel_Parameter_Vector>(default_parameters.data(), default_parameters.data() + default_parameters.size());
		mUpper_Bound = refcnt::Create_Container_shared<double, scgms::SModel_Parameter_Vector>(upper_bound.data(), upper_bound.data() + upper_bound.size());
		
		//find the proper reference id
		for (size_t i = 0; i < desc.number_of_calculated_signals; i++) {
			if (desc.calculated_signal_ids[i] == mCalculated_Signal_Id) {
				mReference_Signal_Id = desc.reference_signal_ids[i];
				break;
			}
		}
	}
	else {
		std::wstring str = dsCannot_Get_Model_Descriptor_By_Signal_Id;
		str += GUID_To_WString(mCalculated_Signal_Id);
		error_description.push(str);
		return E_ACCESSDENIED;
	}
	
	mMetric_Id = configuration.Read_GUID(rsSelected_Metric);
	mUse_Relative_Error = configuration.Read_Bool(rsUse_Relative_Error, mUse_Relative_Error);
	mUse_Squared_Differences = configuration.Read_Bool(rsUse_Squared_Diff, mUse_Squared_Differences);
	mPrefer_More_Levels = configuration.Read_Bool(rsUse_Prefer_More_Levels, mPrefer_More_Levels);
	mMetric_Threshold = configuration.Read_Double(rsMetric_Threshold);
	mUse_Measured_Levels = configuration.Read_Bool(rsUse_Measured_Levels, mUse_Measured_Levels);
	mLevels_Required = configuration.Read_Int(rsMetric_Levels_Required, desc.total_number_of_parameters);

	if (Is_Invalid_GUID(mCalculated_Signal_Id) || std::isnan(mPrediction_Window)) {
		return E_INVALIDARG;
	}
	if (mSolver_Enabled && (Is_Invalid_GUID(mSolver_Id, mMetric_Id) || std::isnan(mMetric_Threshold))) {
		return E_INVALIDARG;
	}

	return S_OK;
}

HRESULT CCalculate_Filter::Do_Execute(scgms::UDevice_Event event)  {
	
	bool event_already_sent = false;
	HRESULT result = E_UNEXPECTED;

	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Level:
		{
			//copy those values, which may be gone once we send the event in the original order
			const uint64_t segment_id = event.segment_id();
			const GUID signal_id = event.signal_id();
			const double level = event.level();
			const double device_time = event.device_time();

			Schedule_Solving(signal_id);

			//send the original event before other events are emitted
			result = mOutput.Send(event);
			if (Succeeded(result)) {
				//now, evt is gone!
				event_already_sent = true;
				Add_Level(segment_id, signal_id, level, device_time);
			}
			else {
				return result;
			}
			break;
		}

		case scgms::NDevice_Event_Code::Parameters:
		{
			if ((!mWarm_Reset_Done && mSolver_Enabled) || (!mSolver_Enabled)) {

				//we either do not solve and therefore we do not preserve calculated parameters
				//or, we calculate parameters and therefore we stop accepting new ones once warm-resetted

				if (event.signal_id() == mCalculated_Signal_Id) {
					if (event.segment_id() != scgms::Invalid_Segment_Id) {

						auto test_and_apply_parameters = [&event, this](const std::unique_ptr<CTime_Segment>& segment) {
							//do these parameters improve?
							if (Is_Invalid_GUID(mMetric_Id)) {
								return true;	//if there's no metric set, we believe the parameters as we got them
							}

							scgms::SMetric metric{ scgms::TMetric_Parameters{ mMetric_Id, bool_2_uc(mUse_Relative_Error),  bool_2_uc(mUse_Squared_Differences), bool_2_uc(mPrefer_More_Levels),  mMetric_Threshold } };
							scgms::ITime_Segment* segment_raw = segment.get();
							const double current_fitness = Calculate_Fitness(&segment_raw, 1, metric, segment->Get_Parameters().get());
							const double new_fitness = Calculate_Fitness(&segment_raw, 1, metric, event.parameters.get());

							if (new_fitness < current_fitness) {
								segment->Set_Parameters(event.parameters);
							}

							return true; // does this lambda really need to return anything?
						};

						if (event.segment_id() != scgms::All_Segments_Id) {
							const auto& segment = Get_Segment(event.segment_id());
							if (segment) {
								test_and_apply_parameters(segment);
							}
						}
						else {
							for (const auto& segment : mSegments) {
								test_and_apply_parameters(segment.second);
							}
						}

						Add_Parameters_Hint(event.parameters);
					}
				}
			}
			break;
		}
		case scgms::NDevice_Event_Code::Parameters_Hint:
			if (event.signal_id() == mCalculated_Signal_Id) {
				Add_Parameters_Hint(event.parameters);
			}
			break;
		case scgms::NDevice_Event_Code::Time_Segment_Stop:
			if (mSolver_Enabled && mSolve_On_Time_Segment_End) {
				mTriggered_Solver_Time = event.device_time();
				Run_Solver(event.segment_id());
			}
			//in this particular case, we do not preserve the original order of events
			//to emit the paramters before the time segment actually ends
			break;
		case scgms::NDevice_Event_Code::Solve_Parameters:
		{
			if (event.signal_id() == scgms::signal_All || event.signal_id() == mCalculated_Signal_Id) {
				// note that the Run_Solver method can handle Any_Segment_Id case properly, so we don't need to disambiguate here
				const auto segment_id = event.segment_id();
				result = mOutput.Send(event);
				event_already_sent = result == S_OK;	//preserve the original order of the events
				mTriggered_Solver_Time = event.device_time();
				Run_Solver(segment_id);
			}
			break;
		}

		case scgms::NDevice_Event_Code::Warm_Reset:
			for (const auto& segment : mSegments) {
				segment.second->Clear_Data();
			}
			mWarm_Reset_Done = true;
			break;
		default:
			break;
	}

	if (!event_already_sent) {
		result = mOutput.Send(event);
	}
	
	return result;
}

void CCalculate_Filter::Add_Level(const uint64_t segment_id, const GUID &signal_id, const double level, const double time_stamp) {

	if ((signal_id == Invalid_GUID) || (signal_id == mCalculated_Signal_Id)) {
		return;	//cannot add what unknown signal and cannot add what we have to compute
	}
	if (segment_id == scgms::Invalid_Segment_Id || segment_id == scgms::All_Segments_Id) {
		return;
	}

	const auto &segment = Get_Segment(segment_id);
	if (segment) {
		if (segment->Update_Level(signal_id, level, time_stamp)) {
			if (mSolving_Scheduled) {
				mTriggered_Solver_Time = time_stamp;
				Run_Solver(segment_id);
			}
			segment->Emit_Levels_At_Pending_Times();
		}
	}
}

void CCalculate_Filter::Add_Parameters_Hint(scgms::SModel_Parameter_Vector parameters) {
	scgms::SModel_Parameter_Vector hint;
	if (hint.set(parameters)) {
		mParameter_Hints.push_back(hint);	//push deep copy as the source may be gone unexpectedly
	}
}

void CCalculate_Filter::Schedule_Solving(const GUID &level_signal_id) {

	if (!mSolver_Enabled) {
		return;
	}

	mSolving_Scheduled = false;

	if (level_signal_id == mReference_Signal_Id) {
		mReference_Level_Counter++;
	}
	if ((mReference_Level_Threshold_Count > 0) && (mReference_Level_Counter >= mReference_Level_Threshold_Count)) {
		mReference_Level_Counter = 0;
		mSolving_Scheduled = true;
	}

	if ((level_signal_id == scgms::signal_BG_Calibration) && mSolve_On_Calibration) {
		mSolving_Scheduled = true;
	}
}

double CCalculate_Filter::Calculate_Fitness(scgms::ITime_Segment **segments, const size_t segment_count, scgms::SMetric metric, scgms::IModel_Parameter_Vector *parameters) {
	metric->Reset();

	size_t real_levels_required = mLevels_Required;
	size_t global_count = 0;
	std::vector<double> reference, times, calculated;

	for (size_t i = 0; i < segment_count; i++) {
		scgms::STime_Segment segment = refcnt::make_shared_reference_ext<scgms::STime_Segment, scgms::ITime_Segment>(segments[i], true);
		auto reference_signal = segment.Get_Signal(mReference_Signal_Id);
		auto calculated_signal = segment.Get_Signal(mCalculated_Signal_Id);

		if (!reference_signal || !calculated_signal) {
			return std::numeric_limits<double>::max();
		}

		size_t count, filled;
		if (reference_signal->Get_Discrete_Bounds(nullptr, nullptr, &count) == S_OK) {
			global_count += count;

			reference.resize(count);
			times.resize(count);
			calculated.resize(count);
			if (reference_signal->Get_Discrete_Levels(times.data(), reference.data(), count, &filled) == S_OK) {
				if (!mUse_Measured_Levels) {
					reference_signal->Get_Continuous_Levels(nullptr, times.data(), reference.data(), count, scgms::apxNo_Derivation);
				}

				if (calculated_signal->Get_Continuous_Levels(parameters, times.data(), calculated.data(), count, scgms::apxNo_Derivation) == S_OK) {
					metric->Accumulate(times.data(), reference.data(), calculated.data(), count);
				}
			}
		}
	}


	if (mLevels_Required < 0) { //get the number of levels required
		//handle the relative value
		real_levels_required = static_cast<size_t>(static_cast<int64_t>(global_count) + mLevels_Required);
	}

	double fitness;
	size_t accumulated = 0;
	if (metric->Calculate(&fitness, &accumulated, real_levels_required) != S_OK) {
		fitness = std::numeric_limits<double>::max();
	}

	return fitness;
}

void CCalculate_Filter::Run_Solver(const uint64_t segment_id) {
	//1. we need to calculate present fitness of current parameters
	//2. then, we attempt to calculate new parameters 
	//3. subsequently, we calculate fitness of the new parameters
	//4. eventually, we apply new parameters if they present a better fitness

	mSolver_Status = scgms::TSolver_Status::In_Progress;

	scgms::SMetric metric{ scgms::TMetric_Parameters{ mMetric_Id, bool_2_uc(mUse_Relative_Error),  bool_2_uc(mUse_Squared_Differences), bool_2_uc(mPrefer_More_Levels),  mMetric_Threshold } };

	auto solve_segment = [this, &metric, segment_id](scgms::ITime_Segment **segments, const size_t segment_count, scgms::SModel_Parameter_Vector working_parameters) {
		
		size_t real_levels_required = 0;
		{	//get the number of levels required
			size_t global_count = 0;
			if (mLevels_Required < 0) {
				//handle the relative value

				for (size_t i = 0; i < segment_count; i++) {
					scgms::ISignal *signal;
					if (segments[i]->Get_Signal(&mReference_Signal_Id, &signal) == S_OK) {
						size_t local_count = 0;
						if (signal->Get_Discrete_Bounds(nullptr, nullptr, &local_count) == S_OK) {
							global_count += local_count;
						}
					}
				}
			}
			real_levels_required = static_cast<size_t>(static_cast<int64_t>(global_count) + mLevels_Required);
		}

		//get the raw hints
		std::vector<scgms::IModel_Parameter_Vector*> raw_hints;
		{
			for (auto& hint : mParameter_Hints) {
				raw_hints.push_back(hint.get());
			}
			if (mDefault_Parameters) {
				raw_hints.push_back(mDefault_Parameters.get());
			}

			//find the best parameters
			if (raw_hints.size()>1) {
				double best_fitness = Calculate_Fitness(segments, segment_count, metric, raw_hints[0]);
				size_t best_index = 0;
				for (size_t i = 1; i < raw_hints.size(); i++) {
					const double local_fitness = Calculate_Fitness(segments, segment_count, metric, raw_hints[i]);
					if (local_fitness < best_fitness) {
						best_index = i;
					}
				}

				//swap the best hint to the first position
				if (best_index > 0) {
					std::swap(raw_hints[0], raw_hints[best_index]);
				}
			}
		}

		metric->Reset();
		mSolver_Progress = solver::Null_Solver_Progress;
		
		scgms::SModel_Parameter_Vector solved_parameters{ mDefault_Parameters };

		TSegment_Solver_Setup setup{
			mSolver_Id, mCalculated_Signal_Id, mReference_Signal_Id,
			segments, segment_count,
			metric.get(), real_levels_required, bool_2_uc(mUse_Measured_Levels),
			mLower_Bound.get(), mUpper_Bound.get(),
			raw_hints.data(), raw_hints.size(),
			solved_parameters.get(),
			&mSolver_Progress
		};

		if (Solve_Model_Parameters(setup) == S_OK) {
			//calculate and compare the present parameters with the new one

			const double original_fitness = Calculate_Fitness(segments, segment_count, metric, working_parameters.get());
			const double solved_fitness = Calculate_Fitness(segments, segment_count, metric, solved_parameters.get());
			if (solved_fitness < original_fitness) {
				mSolver_Status = scgms::TSolver_Status::Completed_Improved;

				//OK, we have found better fitness => set it and send respective message
				if (segment_id != scgms::All_Segments_Id) {
					const auto &segment = Get_Segment(segment_id);
					if (segment) {
						segment->Set_Parameters(solved_parameters);
					}
				}
				else {
					for (auto& segment : mSegments) {
						segment.second->Set_Parameters(solved_parameters);
					}
				}

				Add_Parameters_Hint(solved_parameters);

				scgms::UDevice_Event solved_evt{ scgms::NDevice_Event_Code::Parameters };
				solved_evt.device_time() = mTriggered_Solver_Time;
				solved_evt.device_id() = calculate::Calculate_Filter_GUID;
				solved_evt.signal_id() = mCalculated_Signal_Id;
				solved_evt.segment_id() = segment_id;
				solved_evt.parameters.set(solved_parameters);
				mOutput.Send(solved_evt);
			}
			else {
				mSolver_Status = scgms::TSolver_Status::Completed_Not_Improved;
				scgms::UDevice_Event not_improved_evt{ scgms::NDevice_Event_Code::Information };
				not_improved_evt.device_time() = Unix_Time_To_Rat_Time(time(nullptr));
				not_improved_evt.device_id() = calculate::Calculate_Filter_GUID;
				not_improved_evt.signal_id() = mCalculated_Signal_Id;
				not_improved_evt.segment_id() = segment_id;
				not_improved_evt.info.set(rsInfo_Solver_Completed_But_No_Improvement);
				mOutput.Send(not_improved_evt);
			}

		} else {
			mSolver_Status = scgms::TSolver_Status::Failed;

			//for some reason, it has failed
			scgms::UDevice_Event failed_evt{ scgms::NDevice_Event_Code::Information };
			failed_evt.device_time() = Unix_Time_To_Rat_Time(time(nullptr));
			failed_evt.device_id() = calculate::Calculate_Filter_GUID;
			failed_evt.signal_id() = mCalculated_Signal_Id;
			failed_evt.segment_id() = segment_id;
			failed_evt.info.set(rsInfo_Solver_Failed);
			mOutput.Send(failed_evt);
		}

	};

	//do not forget that CTimeSegment is not reference-counted, so that we can omit all those addrefs and releases
	if (!mSolve_All_Segments) {
		if (segment_id != scgms::All_Segments_Id) {
			//solve just the one, given segment
			const auto segment = Get_Segment(segment_id).get();
			if (segment) {
				scgms::ITime_Segment *raw_segment = static_cast<scgms::ITime_Segment*>(segment);
				solve_segment(&raw_segment, 1, segment->Get_Parameters());
			}
		} else {
			//enumerate all known segments and solve them one by one
			for (auto &segment : mSegments) {
				scgms::ITime_Segment *raw_segment = static_cast<scgms::ITime_Segment*>(segment.second.get());
				solve_segment(&raw_segment, 1, segment.second.get()->Get_Parameters());
			}
		}
	}
	else {
		//solve using all segments
		if (!mSegments.empty()) {
			std::vector<scgms::ITime_Segment*> raw_segments;
			for (auto& segment : mSegments) {
				raw_segments.push_back(static_cast<scgms::ITime_Segment*>(segment.second.get()));
			}

			solve_segment(raw_segments.data(), raw_segments.size(), mSegments.begin()->second->Get_Parameters());
		}
	}
}

HRESULT IfaceCalling CCalculate_Filter::Get_Solver_Progress(solver::TSolver_Progress* const progress) {
	// copy progress structure
	*progress = mSolver_Progress;

	return S_OK;
}

HRESULT IfaceCalling CCalculate_Filter::Get_Solver_Information(GUID* const calculated_signal_id, scgms::TSolver_Status* const status) const {
	*calculated_signal_id = mCalculated_Signal_Id;
	*status = mSolver_Status;

	return S_OK;
}

HRESULT IfaceCalling CCalculate_Filter::Cancel_Solver() {
	mSolver_Progress.cancelled = TRUE;

	return S_OK;
}
