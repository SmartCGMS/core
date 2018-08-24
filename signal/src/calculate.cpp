#include "calculate.h"

#include "descriptor.h"

#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <iostream>

constexpr unsigned char bool_2_uc(const bool b) {
	return b ? static_cast<const unsigned char>(1) : static_cast<const unsigned char>(0);
}


CCalculate_Filter::CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe) : mInput{ inpipe }, mOutput{ outpipe } {	
}

std::unique_ptr<CTime_Segment>& CCalculate_Filter::Get_Segment(const uint64_t segment_id) {		
	const auto iter = mSegments.find(segment_id);

	if (iter != mSegments.end()) return iter->second;
	else {
		std::unique_ptr<CTime_Segment> segment = std::make_unique<CTime_Segment>(segment_id, mCalculated_Signal_Id, mDefault_Parameters, mPrediction_Window, mOutput);		
		const auto ret = mSegments.insert(std::make_pair(segment_id, std::move(segment)));
		return ret.first->second;
	}
}

void CCalculate_Filter::Configure(glucose::SFilter_Parameters shared_configuration) {
	mCalculated_Signal_Id = shared_configuration.Read_GUID(rsSelected_Signal);
	mPrediction_Window = shared_configuration.Read_Double(rsPrediction_Window);
	mSolver_Enabled = shared_configuration.Read_Bool(rsSolve_Parameters);
	mSolve_On_Calibration = shared_configuration.Read_Bool(rsSolve_On_Calibration);
	mSolve_All_Segments = shared_configuration.Read_Bool(rsSolve_Using_All_Segments);
	mReference_Level_Threshold_Count = shared_configuration.Read_Int(rsSolve_On_Level_Count);
	mSolving_Scheduled = false;
	mReference_Level_Counter = 0;

	mSolver_Id = shared_configuration.Read_GUID(rsSelected_Solver);
	shared_configuration.Read_Parameters(rsSelected_Model_Bounds, mLower_Bound, mDefault_Parameters, mUpper_Bound);	

	//get reference signal id
	glucose::TModel_Descriptor desc = glucose::Null_Model_Descriptor;
	if (glucose::get_model_descriptor_by_signal_id(mCalculated_Signal_Id, desc)) {

		auto set_params = [](glucose::SModel_Parameter_Vector &parameters, const double *begin, const size_t count) {
			if (!parameters) parameters = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(const_cast<double*>(begin), const_cast<double*>(begin) +count);
				else parameters->set(const_cast<double*>(begin), const_cast<double*>(begin) +count);
		};

		if (mLower_Bound.empty()) set_params(mLower_Bound, desc.lower_bound, desc.number_of_parameters);
		if (mDefault_Parameters.empty()) set_params(mDefault_Parameters, desc.default_values, desc.number_of_parameters);
		if (mUpper_Bound.empty()) set_params(mUpper_Bound, desc.upper_bound, desc.number_of_parameters);


		//find the proper reference id		
		for (size_t i = 0; i < desc.number_of_calculated_signals; i++)
			if (desc.calculated_signal_ids[i] == mCalculated_Signal_Id) {
				mReference_Signal_Id = desc.reference_signal_ids[i];
				break;
			}
	}
	
	mMetric_Id = shared_configuration.Read_GUID(rsSelected_Metric);
	mUse_Relative_Error = shared_configuration.Read_Bool(rsUse_Relative_Error);
	mUse_Squared_Differences = shared_configuration.Read_Bool(rsUse_Squared_Diff);
	mPrefer_More_Levels = shared_configuration.Read_Bool(rsUse_Prefer_More_Levels);
	mMetric_Threshold = shared_configuration.Read_Double(rsMetric_Threshold);
	mUse_Measured_Levels = shared_configuration.Read_Bool(rsUse_Measured_Levels);
	mLevels_Required = shared_configuration.Read_Int(rsMetric_Levels_Required);
	
}

HRESULT CCalculate_Filter::Run(glucose::IFilter_Configuration* configuration)  {
	Configure(refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true));

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {

		bool event_already_sent = false;

		switch (evt.event_code) {
			case glucose::NDevice_Event_Code::Level:
				 {					
					//copy those values, which may be gone once we send the event in the original order
					const uint64_t segment_id = evt.segment_id;
					const GUID signal_id = evt.signal_id;
					const double level = evt.level;
					const double device_time = evt.device_time;

					Schedule_Solving(signal_id);

					//send the original event before other events are emitted
					if (mOutput.Send(evt)) {
						//now, evt may be gone!
						event_already_sent = true;
						Add_Level(segment_id, signal_id, level, device_time);					
					}
					else
						break;
				}
				break;

			case glucose::NDevice_Event_Code::Parameters:

				if ((!mWarm_Reset_Done && mSolver_Enabled) || (!mSolver_Enabled))

					//we either do not solve and therefore we do not preserve calculated parameters
					//or, we calculate parameters and therefore we stop accepting new ones once warm-resetted

					if (evt.signal_id == mCalculated_Signal_Id) {
						if (evt.segment_id != glucose::Invalid_Segment_Id) {
							const auto &segment = Get_Segment(evt.segment_id);
							if (segment) {
								//do these parameters improve?
								glucose::SMetric metric{ glucose::TMetric_Parameters{ mMetric_Id, bool_2_uc(mUse_Relative_Error),  bool_2_uc(mUse_Squared_Differences), bool_2_uc(mPrefer_More_Levels),  mMetric_Threshold } };
								glucose::ITime_Segment *segment_raw = segment.get();
								const double current_fitness = Calculate_Fitness(&segment_raw, 1, metric, segment->Get_Parameters().get());
								const double new_fitness = Calculate_Fitness(&segment_raw, 1, metric, evt.parameters.get());

								if (new_fitness < current_fitness)
									segment->Set_Parameters(evt.parameters);

							}
							Add_Parameters_Hint(evt.parameters);
						}
					}
				break;

			case glucose::NDevice_Event_Code::Parameters_Hint:
				if (evt.signal_id == mCalculated_Signal_Id) Add_Parameters_Hint(evt.parameters);
				break;

			case glucose::NDevice_Event_Code::Solve_Parameters: {
					const auto segment_id = evt.segment_id;
					event_already_sent = mOutput.Send(evt);	//preserve original order of the events
					Run_Solver(segment_id);
				}
				break;

			case glucose::NDevice_Event_Code::Warm_Reset:
				for (const auto &segment : mSegments)
					segment.second->Clear_Data();
				mWarm_Reset_Done = true;
				break;
			default:
				break;
		}


		if (!event_already_sent)
			if (!mOutput.Send(evt))
				break;
	}

	return S_OK;
}

void CCalculate_Filter::Add_Level(const uint64_t segment_id, const GUID &signal_id, const double level, const double time_stamp) {

	if ((signal_id == Invalid_GUID) || (signal_id == mCalculated_Signal_Id)) return;	//cannot add what unknown signal and cannot add what we have to compute
	if (segment_id == glucose::Invalid_Segment_Id) return;

	const auto &segment = Get_Segment(segment_id);
	if (segment)
		if (segment->Add_Level(signal_id, level, time_stamp)) {
			if (mSolving_Scheduled) Run_Solver(segment_id);
			segment->Emit_Levels_At_Pending_Times();
		}
}


void CCalculate_Filter::Add_Parameters_Hint(glucose::SModel_Parameter_Vector parameters) {
	glucose::SModel_Parameter_Vector hint;
	if (hint.set(parameters)) {
		mParameter_Hints.push_back(hint);	//push deep copy as the source may be gone unexpectedly
	}
}

void CCalculate_Filter::Schedule_Solving(const GUID &level_signal_id) {
	mSolving_Scheduled = false;

	if (level_signal_id == mReference_Signal_Id) mReference_Level_Counter++;
	if ((mReference_Level_Threshold_Count > 0) && (mReference_Level_Counter >= mReference_Level_Threshold_Count)) {
		mReference_Level_Counter = 0;
		mSolving_Scheduled = true;
	}

	if ((level_signal_id == glucose::signal_Calibration) && mSolve_On_Calibration) mSolving_Scheduled = true;
}

double CCalculate_Filter::Calculate_Fitness(glucose::ITime_Segment **segments, const size_t segment_count, glucose::SMetric metric, glucose::IModel_Parameter_Vector *parameters) {
	metric->Reset();

	size_t real_levels_required = mLevels_Required;
	size_t global_count = 0;
	std::vector<double> reference, times, calculated;
	for (size_t i = 0; i < segment_count; i++) {
		glucose::STime_Segment segment = refcnt::make_shared_reference_ext<glucose::STime_Segment, glucose::ITime_Segment>(segments[i], true);
		auto reference_signal = segment.Get_Signal(mReference_Signal_Id);
		auto calculated_signal = segment.Get_Signal(mCalculated_Signal_Id);

		if (!reference_signal || !calculated_signal) return std::numeric_limits<double>::max();

		glucose::TBounds bounds;
		size_t count, filled;
		if (reference_signal->Get_Discrete_Bounds(&bounds, &count) == S_OK) {
			global_count += count;


			reference.resize(count);
			times.resize(count);
			calculated.resize(count);
			if (reference_signal->Get_Discrete_Levels(times.data(), reference.data(), count, &filled) == S_OK) {
				if (!mUse_Measured_Levels)
					reference_signal->Get_Continuous_Levels(nullptr, times.data(), reference.data(), count, glucose::apxNo_Derivation);


				if (calculated_signal->Get_Continuous_Levels(parameters, times.data(), calculated.data(), count, glucose::apxNo_Derivation) == S_OK) {
					metric->Accumulate(times.data(), reference.data(), calculated.data(), count);
				}
			}
		}
	}


	if (mLevels_Required < 0) 	//get the number of levels required
		//handle the relative value
		real_levels_required = static_cast<size_t>(static_cast<int64_t>(global_count) + mLevels_Required);


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

	

	glucose::SMetric metric{ glucose::TMetric_Parameters{ mMetric_Id, bool_2_uc(mUse_Relative_Error),  bool_2_uc(mUse_Squared_Differences), bool_2_uc(mPrefer_More_Levels),  mMetric_Threshold } };

	auto solve_segment = [this, &metric, segment_id](glucose::ITime_Segment **segments, const size_t segment_count, glucose::SModel_Parameter_Vector working_parameters) {
		
		size_t real_levels_required = 0;		
		{	//get the number of levels required
			size_t global_count = 0;
			if (mLevels_Required < 0) {
				//handle the relative value

				for (size_t i = 0; i < segment_count; i++) {
					glucose::ISignal *signal;
					if (segments[i]->Get_Signal(&mReference_Signal_Id, &signal) == S_OK) {
						size_t local_count = 0;
						if (signal->Get_Discrete_Bounds(nullptr, &local_count) == S_OK)
							global_count += local_count;
					}
				}
			}
			real_levels_required = static_cast<size_t>(static_cast<int64_t>(global_count) + mLevels_Required);
		}

		std::vector<glucose::IModel_Parameter_Vector*> raw_hints;
		{	//get the raw hints
			for (auto &hint : mParameter_Hints)
				raw_hints.push_back(hint.get());
			if (mDefault_Parameters) raw_hints.push_back(mDefault_Parameters.get());

			//find the best parameters
			if (raw_hints.size()>1) {
				double best_fitness = Calculate_Fitness(segments, segment_count, metric, raw_hints[0]);
				size_t best_index = 0;
				for (size_t i = 1; i < raw_hints.size(); i++) {
					const double local_fitness = Calculate_Fitness(segments, segment_count, metric, raw_hints[i]);
					if (local_fitness < best_fitness) best_index = i;
				}

				//swap the best hint to the first position
				if (best_index>0) std::swap(raw_hints[0], raw_hints[best_index]);
			}
		}

		metric->Reset();
		glucose::TSolver_Progress progress{ 0 };
		
		glucose::SModel_Parameter_Vector solved_parameters{ mDefault_Parameters };

		glucose::TSolver_Setup setup{
			mSolver_Id, mCalculated_Signal_Id, mReference_Signal_Id,
			segments, segment_count,
			metric.get(), real_levels_required, bool_2_uc(mUse_Measured_Levels),
			mLower_Bound.get(), mUpper_Bound.get(),		
			raw_hints.data(), raw_hints.size(),
			solved_parameters.get(),
			&progress
		};

		if (glucose::Solve_Model_Parameters(setup) == S_OK) {
			//calculate and compare the present parameters with the new one

			const double original_fitness = Calculate_Fitness(segments, segment_count, metric, working_parameters.get());
			const double solved_fitness = Calculate_Fitness(segments, segment_count, metric, solved_parameters.get());
			if (solved_fitness < original_fitness) {
				//OK, we have found better fitness => set it and send respective message
				if (segment_id != glucose::Invalid_Segment_Id) {
					const auto &segment = Get_Segment(segment_id);
					if (segment) 
						segment->Set_Parameters(solved_parameters);					
				}
				else {
					for (auto &segment : mSegments)
						segment.second->Set_Parameters(solved_parameters);
				}
				
				Add_Parameters_Hint(solved_parameters);
				

				glucose::UDevice_Event solved_evt{ glucose::NDevice_Event_Code::Parameters };
				solved_evt.device_time = Unix_Time_To_Rat_Time(time(nullptr));
				solved_evt.device_id = calculate::Calculate_Filter_GUID;
				solved_evt.signal_id = mCalculated_Signal_Id;
				solved_evt.segment_id = segment_id;
				solved_evt.parameters.set(solved_parameters);				
				mOutput.Send(solved_evt);
			}

		} else {
			//for some reason, it has failed
			glucose::UDevice_Event failed_evt{ glucose::NDevice_Event_Code::Information };
			failed_evt.device_time = Unix_Time_To_Rat_Time(time(nullptr));
			failed_evt.device_id = calculate::Calculate_Filter_GUID;
			failed_evt.signal_id = mCalculated_Signal_Id;			
			failed_evt.segment_id = segment_id;
			failed_evt.info.set(rsInfo_Solver_Failed);
			mOutput.Send(failed_evt);
		}

	};

	//do not forget that CTimeSegment is not reference-counted, so that we can omit all those addrefs and releases
	if (!mSolve_All_Segments) {
		if (segment_id != glucose::Invalid_Segment_Id) {
			//solve just the one, given segment
			const auto segment = Get_Segment(segment_id).get();			
			if (segment) {
				glucose::ITime_Segment *raw_segment = static_cast<glucose::ITime_Segment*>(segment);
				solve_segment(&raw_segment, 1, segment->Get_Parameters());
			}
		} else {
			//enumerate all known segments and solve them one by one
			for (auto &segment : mSegments) {
				glucose::ITime_Segment *raw_segment = static_cast<glucose::ITime_Segment*>(segment.second.get());
				solve_segment(&raw_segment, 1, segment.second.get()->Get_Parameters());
			}
		}
	}
	else {
		//solve using all segments
		if (!mSegments.empty()) {
			std::vector<glucose::ITime_Segment*> raw_segments;
			for (auto &segment : mSegments) 
				raw_segments.push_back(static_cast<glucose::ITime_Segment*>(segment.second.get()));

			solve_segment(raw_segments.data(), raw_segments.size(), mSegments.begin()->second->Get_Parameters());			
		}
	}
}