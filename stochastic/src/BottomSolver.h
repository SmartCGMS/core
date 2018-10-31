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

#include <vector>
#include <random>


#include "solution.h"
#include "fitness.h"
#include "nlopt.h"
#include "LocalSearch.h"


#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>

#undef max


template <typename TResult_Solution, typename TFitness, typename TTop_Solution = TResult_Solution, typename TBottom_Solution = TResult_Solution, typename TBottom_Solver = CNullMethod<TResult_Solution, CNLOpt_Fitness_Proxy<TFitness, TResult_Solution, TResult_Solution>>>
class CBottom_Solver {
protected:
	TBottom_Solution mLower_Bottom_Bound;
	TBottom_Solution mUpper_Bottom_Bound;
	TResult_Solution mInitial_Solution;
protected:
	TFitness &mFitness;
	SMetricFactory &mMetric_Factory;
public:
	CBottom_Solver(const TAligned_Solution_Vector<TResult_Solution> &initial_solutions, const TResult_Solution &lower_bound, const TResult_Solution &upper_bound, TFitness &fitness, SMetricFactory &metric_factory) :
		mFitness(fitness), mMetric_Factory(metric_factory) {

		if (initial_solutions.size() > 0) mInitial_Solution = initial_solutions[0];

		TTop_Solution lower_top_bound, upper_top_bound;
		mLower_Bottom_Bound = lower_top_bound.Decompose(lower_bound);
		mUpper_Bottom_Bound = upper_top_bound.Decompose(upper_bound);
	}


	TResult_Solution Solve(volatile TSolverProgress &progress) {

		TTop_Solution top_result;
		const TAligned_Solution_Vector<TBottom_Solution> bottom_init = { top_result.Decompose(mInitial_Solution) };	//also initializes top_results
		CNLOpt_Fitness_Proxy<TFitness, TTop_Solution, TBottom_Solution> fitness_proxy{ mFitness, top_result }; //that's needed by fitness_proxy
		TBottom_Solver bottom_solver(bottom_init, mLower_Bottom_Bound, mUpper_Bottom_Bound, fitness_proxy, mMetric_Factory);
		TBottom_Solution bottom_result = bottom_solver.Solve(progress);

		return top_result.Compose(bottom_result);
	}
};
