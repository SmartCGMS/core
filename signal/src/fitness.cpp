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

#include "fitness.h"

#include <scgms/rtl/SolverLib.h>
#include <scgms/rtl/UILib.h>
#include <scgms/rtl/referencedImpl.h>

#include <numeric>
#include <execution> 

thread_local scgms::SMetric CFitness::mMetric_Per_Thread;
thread_local aligned_double_vector CFitness::mTemporal_Levels;

CFitness::CFitness(const TSegment_Solver_Setup &setup, const size_t solution_size)
	: mLevels_Required(setup.levels_required), mSolution_Size(solution_size) {

	setup.metric->Get_Parameters(&mMetric_Params);	//shall it fail, it will fail down there

	mMax_Levels_Per_Segment = 0;

	for (size_t segment_iter = 0; segment_iter < setup.segment_count; segment_iter++) {

		std::shared_ptr<scgms::ITime_Segment> setup_segment= refcnt::make_shared_reference<scgms::ITime_Segment>(setup.segments[segment_iter], true);

		TSegment_Info info{ nullptr, nullptr, nullptr, {}, {} };
		info.segment = setup_segment;

		scgms::ISignal *signal;
		if (setup_segment->Get_Signal(&setup.calculated_signal_id, &signal) == S_OK) {
			info.calculated_signal = refcnt::make_shared_reference<scgms::ISignal>(signal, false);
		}

		if (setup_segment->Get_Signal(&setup.reference_signal_id, &signal) == S_OK) {
			info.reference_signal = refcnt::make_shared_reference<scgms::ISignal>(signal, false);
		}

		if (info.calculated_signal && info.reference_signal) {

			size_t levels_count;
			if (info.reference_signal->Get_Discrete_Bounds(nullptr, nullptr, &levels_count) == S_OK && levels_count > 0) {
				info.reference_time.resize(levels_count);
				info.reference_level.resize(levels_count);
					
				//prepare arrays with reference levels and their times
				if (info.reference_signal->Get_Discrete_Levels(info.reference_time.data(), info.reference_level.data(), info.reference_time.size(), &levels_count) == S_OK) {
					info.reference_time.resize(levels_count);
					info.reference_level.resize(levels_count);
				}

				//if desired, replace them continous signal approdximation
				if (setup.use_measured_levels == 0) {
					info.reference_signal->Get_Continuous_Levels(nullptr, info.reference_time.data(), info.reference_level.data(), info.reference_time.size(), scgms::apxNo_Derivation);
					//we are not interested in checking the possibly error, because we will be left with meeasured levels at least
				}

				//do we have everyting we need to test this segment?
				if (!info.reference_time.empty()) {
					mMax_Levels_Per_Segment = std::max(mMax_Levels_Per_Segment, info.reference_time.size());
					mSegment_Info.push_back(info);
				}
			}

		} //if we managed to get both segment and signal
	} //for each segments

} //ctor's end

double* CFitness::Reserve_Temporal_Levels_Data() {
	if (mTemporal_Levels.size() < mMax_Levels_Per_Segment) {
		mTemporal_Levels.resize(mMax_Levels_Per_Segment);
	}

	return mTemporal_Levels.data();
}

double CFitness::Calculate_Fitness(const double *solution) {
	//caller has to supply the metric to allow parallelization without inccuring an overhead
	//of creating new metric calculator instead of its mere resetting	

	scgms::IMetric *metric = mMetric_Per_Thread.get();	//caching to avoid TLS calls
	if (!metric) {
		mMetric_Per_Thread = scgms::SMetric{ mMetric_Params };
		metric = mMetric_Per_Thread.get();
		if (!metric) {
			return std::numeric_limits<double>::quiet_NaN();
		}
	}

	metric->Reset();	//could have been called by else, but it would happen only few times in the beginning => save code size and BTB slots

	//let's pick a memory block for calculated
	double* const tmp_levels = Reserve_Temporal_Levels_Data();

	refcnt::internal::CVector_View<double> solution_view{ solution, solution + mSolution_Size };

	for (auto &info : mSegment_Info) {
		if (info.calculated_signal->Get_Continuous_Levels(&solution_view, info.reference_time.data(), tmp_levels, info.reference_time.size(), scgms::apxNo_Derivation) == S_OK) {
			//levels got, calculate the metric
			metric->Accumulate(info.reference_time.data(), info.reference_level.data(), tmp_levels, info.reference_time.size());
		}
		else {
			//quit immediatelly as the paramters must be valid for all segments
			return std::numeric_limits<double>::max();
		}
	}

	//eventually, calculate the metric number
	double result;
	size_t tmp_size;
	if (metric->Calculate(&result, &tmp_size, mLevels_Required) != S_OK) {
		result = std::numeric_limits<double>::max();
	}

	return result;
}

BOOL IfaceCalling Fitness_Wrapper(const void* data, const size_t solution_count, const double* solutions, double* const fitnesses) {
	CFitness *candidate = reinterpret_cast<CFitness*>(const_cast<void*>(data));
	
	if (solution_count > 1) {
		std::for_each(std::execution::par_unseq, solver::CInt_Iterator<size_t>{ 0 }, solver::CInt_Iterator<size_t>{ solution_count }, [=](const auto& id) {
			fitnesses[id] = candidate->Calculate_Fitness(solutions + id * candidate->mSolution_Size);
		});
	} else {
		*fitnesses = candidate->Calculate_Fitness(solutions);
	}
	return TRUE;
}

HRESULT Solve_Model_Parameters(const TSegment_Solver_Setup &setup) {	
	HRESULT rc = E_UNEXPECTED;
	const auto model_descriptors = scgms::get_model_descriptor_list();

	/*if (rc != S_OK) */{
		//let's try to apply the generic filters as well
		scgms::TModel_Descriptor *model = nullptr;
		for (size_t desc_idx = 0; desc_idx < model_descriptors.size(); desc_idx++) {
			for (size_t signal_idx = 0; signal_idx < model_descriptors[desc_idx].number_of_calculated_signals; signal_idx++) {
				if (model_descriptors[desc_idx].calculated_signal_ids[signal_idx] == setup.calculated_signal_id) {
					model = const_cast<scgms::TModel_Descriptor*>(&model_descriptors[desc_idx]);
					break;
				}
			}
		}

		if (model != nullptr) {
			double *lower_bound, *upper_bound, *begin, *end;

			if ((setup.lower_bound->get(&lower_bound, &end) != S_OK) || (setup.upper_bound->get(&upper_bound, &end) != S_OK)) {
				return E_FAIL;
			}

			std::vector<double*> hints;
			for (size_t i = 0; i < setup.hint_count; i++) {
				if (setup.solution_hints[i]->get(&begin, &end) == S_OK) {
					hints.push_back(begin);
				}
			}

			std::vector<double> solution(model->total_number_of_parameters);

			CFitness fitness{ setup, model->total_number_of_parameters};

			solver::TSolver_Setup generic_setup{
				model->total_number_of_parameters,
				1,	//TSegment_Solver_Setup does not allow multi-objective optimization
				lower_bound, upper_bound,
				const_cast<const double**> (hints.data()), hints.size(),
				solution.data(),


				&fitness,
				Fitness_Wrapper, nullptr,

				solver::Default_Solver_Setup.max_generations,
				solver::Default_Solver_Setup.population_size,
				solver::Default_Solver_Setup.tolerance,
			};

			rc = solver::Solve_Generic(setup.solver_id, generic_setup, *setup.progress);
			if (rc == S_OK) {
				rc = setup.solved_parameters->set(solution.data(), solution.data()+solution.size());
			}

			return rc;
		}
	}

	return rc;
}
