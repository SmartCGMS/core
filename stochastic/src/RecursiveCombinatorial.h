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


#include "LocalSearch.h"


template <typename TSolution, typename TFitness, bool _Preserve_Combination_Order>
class CRecursive_Combinatorial_Descent {
protected:
	const size_t mCombinatorial_Granularity = 10;
	const floattype mScale_Factor = 0.1;
protected:
	const TSolution mLower_Bound;
	const TSolution mUpper_Bound;
	TAligned_Solution_Vector<TSolution> mInitial_Solutions;
protected:
	TFitness &mFitness;
	SMetricFactory &mMetric_Factory;
public:
	CRecursive_Combinatorial_Descent(const TAligned_Solution_Vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, SMetricFactory &metric_factory) :
		mInitial_Solutions(initial_solutions), mLower_Bound(lower_bound), mUpper_Bound(upper_bound), mFitness(fitness), mMetric_Factory(metric_factory) {
	}


	TSolution Solve(volatile TSolverProgress &progress) {


		TSolution local_lower_bound = mLower_Bound;
		TSolution local_upper_bound = mUpper_Bound;
		
		size_t iter_count = 100;

		decltype(mInitial_Solutions) local_solutions = { mInitial_Solutions };

		TSolution previous_local_result = mInitial_Solutions[0];
		floattype previous_local_fitness = mFitness.Calculate_Fitness(previous_local_result, mMetric_Factory.CreateCalculator());

		progress.MaxProgress = 100;
		progress.CurrentProgress = 0;

		while (iter_count-- > 0) {
			CLocalSearch<TSolution, TFitness, _Preserve_Combination_Order, _Preserve_Combination_Order ? 20 : 100'000> local_search(local_solutions, local_lower_bound, local_upper_bound, mFitness, mMetric_Factory);
			TSolverProgress tmp_progress;
			TSolution present_local_result = local_search.Solve(tmp_progress);

			floattype present_local_fitness = mFitness.Calculate_Fitness(present_local_result, mMetric_Factory.CreateCalculator());
			if (present_local_fitness >= previous_local_fitness) break;

			previous_local_result = present_local_result;
			previous_local_fitness = present_local_fitness;


			local_lower_bound = mLower_Bound.max(present_local_result*(1.0 - (1.0 / static_cast<floattype>(mScale_Factor))));
			local_upper_bound = mUpper_Bound.min(present_local_result*(1.0 + (1.0 / static_cast<floattype>(mScale_Factor))));

			local_solutions.clear();
			local_solutions.push_back(present_local_result);

			progress.CurrentProgress++;
			progress.BestMetric = present_local_fitness;
		}

		return previous_local_result;
	}

};
