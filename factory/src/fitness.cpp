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

#include "filters.h"
#include "fitness.h"
#include "../../../common/rtl/referencedImpl.h"

thread_local glucose::SMetric CFitness::mMetric_Per_Thread;

CFitness::CFitness(const glucose::TSolver_Setup &setup, const size_t solution_size) : mLevels_Required (setup.levels_required), mSolution_Size(solution_size){

	setup.metric->Get_Parameters(&mMetric_Params);	//shall it fail, it will fail down there

	mMax_Levels_Per_Segment = 0;	

		
	for (size_t segment_iter=0; segment_iter<setup.segment_count;  segment_iter++) {
			
		std::shared_ptr<glucose::ITime_Segment> setup_segment= refcnt::make_shared_reference<glucose::ITime_Segment>(setup.segments[segment_iter], true);


		TSegment_Info info{ nullptr, nullptr, nullptr};
		info.segment = setup_segment;

		glucose::ISignal *signal;
		if (setup_segment->Get_Signal(&setup.calculated_signal_id, &signal) == S_OK)
			info.calculated_signal = refcnt::make_shared_reference<glucose::ISignal>(signal, false);

		if (setup_segment->Get_Signal(&setup.reference_signal_id, &signal) == S_OK)
			info.reference_signal = refcnt::make_shared_reference<glucose::ISignal>(signal, false);


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
					info.reference_signal->Get_Continuous_Levels(nullptr, info.reference_time.data(), info.reference_level.data(), info.reference_time.size(), glucose::apxNo_Derivation);
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
	if (mTemporal_Levels.size() < mMax_Levels_Per_Segment) mTemporal_Levels.resize(mMax_Levels_Per_Segment);

	return mTemporal_Levels.data();
}

double CFitness::Calculate_Fitness(const double *solution) {
	//caller has to supply the metric to allow parallelization without inccuring an overhead
	//of creating new metric calculator instead of its mere resetting	

	glucose::IMetric *metric = mMetric_Per_Thread.get();	//caching to avoid TLS calls
	if (!metric) {
		mMetric_Per_Thread = glucose::SMetric{ mMetric_Params };
		metric = mMetric_Per_Thread.get();
		if (!metric) return std::numeric_limits<double>::quiet_NaN();
	}

	metric->Reset();	//could have been called by else, but it would happen only few times in the beginning => save code size and BTB slots

	//let's pick a memory block for calculated
	double* const tmp_levels = Reserve_Temporal_Levels_Data();

	refcnt::internal::CVector_View<double> solution_view{ solution, solution + mSolution_Size };

	for (auto &info : mSegment_Info) {
		if (info.calculated_signal->Get_Continuous_Levels(&solution_view, info.reference_time.data(), tmp_levels, info.reference_time.size(), glucose::apxNo_Derivation) == S_OK) {
			//levels got, calculate the metric
			metric->Accumulate(info.reference_time.data(), info.reference_level.data(), tmp_levels, info.reference_time.size());
		} else
			//quit immediatelly as the paramters must be valid for all segments
			return std::numeric_limits<double>::max();
	}

	//eventually, calculate the metric number
	double result;
	size_t tmp_size;
	if (metric->Calculate(&result, &tmp_size, mLevels_Required) != S_OK) result = std::numeric_limits<double>::max();

	return result;
}


double IfaceCalling Fitness_Wrapper(const void *data, const double *solution) {	
	CFitness *fitness = reinterpret_cast<CFitness*>(const_cast<void*>(data));
	return fitness->Calculate_Fitness(solution);
}