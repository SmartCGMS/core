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


#include "../../../common/iface/SolverIface.h"
#include "../../../common/rtl/Buffer_Pool.h"
#include "../../../common/rtl/AlignmentAllocator.h"

#include <tbb/concurrent_queue.h>

#include <vector>


#undef max

using aligned_double_vector = std::vector<double, AlignmentAllocator<double>>;	//Needed for Eigen and SIMD optimizations



struct TSegment_Info {
	std::shared_ptr<glucose::ITime_Segment> segment;
	std::shared_ptr<glucose::ISignal> calculated_signal;
	std::shared_ptr<glucose::ISignal> reference_signal;
	aligned_double_vector reference_time;
	aligned_double_vector reference_level;
};

class CFitness {
protected:
	const size_t mSolution_Size;
	glucose::TMetric_Parameters mMetric = glucose::Null_Metric_Parameters;	
	tbb::concurrent_queue<std::shared_ptr<glucose::IMetric>> mMetric_Pool;
protected:
	std::vector<TSegment_Info> mSegment_Info;	
	size_t mLevels_Required;
	size_t mMax_Levels_Per_Segment;	//to avoid multiple resize of memory block when calculating the error
	CBuffer_Pool<aligned_double_vector> mTemporal_Levels{ [](auto &container, auto minimum_size) {
		if (container.size() < minimum_size) container.resize(minimum_size);
	} };
public:
	CFitness(const glucose::TSolver_Setup &setup, const size_t solution_size);
	double Calculate_Fitness(const double *solution);

};


double IfaceCalling Fitness_Wrapper(const void *data, const double *solution);