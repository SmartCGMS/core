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


#include <scgms/rtl/SolverLib.h>
#include <scgms/rtl/AlignmentAllocator.h>

#include <vector>


#undef max

using aligned_double_vector = std::vector<double, AlignmentAllocator<double>>;	//Needed for Eigen and SIMD optimizations


struct TSegment_Solver_Setup {
	const GUID solver_id; const GUID calculated_signal_id; const GUID reference_signal_id;
	scgms::ITime_Segment** segments; const size_t segment_count;
	scgms::IMetric* metric; const size_t levels_required; const unsigned char use_measured_levels;
	scgms::IModel_Parameter_Vector* lower_bound, * upper_bound;
	scgms::IModel_Parameter_Vector** solution_hints; const size_t hint_count;
	scgms::IModel_Parameter_Vector* solved_parameters;		//obtained result
	solver::TSolver_Progress* progress;
};

struct TSegment_Info {
	std::shared_ptr<scgms::ITime_Segment> segment;
	std::shared_ptr<scgms::ISignal> calculated_signal;
	std::shared_ptr<scgms::ISignal> reference_signal;
	aligned_double_vector reference_time;
	aligned_double_vector reference_level;
};

class CFitness {
protected:	
	scgms::TMetric_Parameters mMetric_Params = scgms::Null_Metric_Parameters;		
	static thread_local scgms::SMetric mMetric_Per_Thread;
protected:
	std::vector<TSegment_Info> mSegment_Info;	
	size_t mLevels_Required;
	size_t mMax_Levels_Per_Segment;	//to avoid multiple resize of memory block when calculating the error
	static thread_local aligned_double_vector mTemporal_Levels;
	double* Reserve_Temporal_Levels_Data();
public:
	const size_t mSolution_Size;

	CFitness(const TSegment_Solver_Setup &setup, const size_t solution_size);
	double Calculate_Fitness(const double *solution);
};


BOOL IfaceCalling Fitness_Wrapper(const void* data, const size_t solution_count, const double* solutions, double* const fitnesses);
HRESULT Solve_Model_Parameters(const TSegment_Solver_Setup &setup);