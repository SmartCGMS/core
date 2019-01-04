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

//Implementation of fast, low-energy cost linear version of the Bayes optimization
//For advanced Bayes, optmization we should consider using the following toolkits (Limbo claims to be faster), which, however, both require Boost
//https://github.com/resibots/limbo
//https://github.com/rmcantin/bayesopt

#pragma once


#include "solution.h"
#include "fitness.h"
#include "HaltonDevice.h"
#include <tbb/parallel_reduce.h>


namespace Halton_internal {
	template <typename TSolution>
	struct TCandidate {
		TSolution solution;
		double fitness;
	};
}


template <typename TSolution, typename TFitness, size_t max_iterations=50, size_t halton_count = 1000>
class CHalton_Sequence {
protected:
	TFitness &mFitness;
	glucose::SMetric &mMetric;
protected:
	const double mZoom_Factor = 0.2;
	TSolution mDefault_Solution;
	TSolution mInitial_Lower_Bound;
	TSolution mInitial_Upper_Bound;
public:
	CHalton_Sequence(const TAligned_Solution_Vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :
		mDefault_Solution(initial_solutions[0]), mInitial_Lower_Bound(lower_bound), mInitial_Upper_Bound(upper_bound), mFitness(fitness), mMetric(metric) {
	};

	TSolution Solve(volatile glucose::TSolver_Progress &progress) {
		TSolution working_lower_bound{ mInitial_Lower_Bound };
		TSolution working_upper_bound { mInitial_Upper_Bound };
	
		mMetric->Reset();		
		Halton_internal::TCandidate<TSolution> best_solution{ working_upper_bound.min(working_lower_bound.max(mDefault_Solution)), 0.0};		
		best_solution.fitness = mFitness.Calculate_Fitness(best_solution.solution, mMetric);

		progress.current_progress = 0;
		progress.max_progress = max_iterations;
		progress.best_metric = best_solution.fitness;

		const size_t solution_size = mDefault_Solution.cols();
		const size_t solution_size_bit_combinations = static_cast<size_t>(1) << solution_size;
		
		std::vector<TSolution, AlignmentAllocator<TSolution>> halton_numbers;
		{
			std::vector<CHalton_Device> halton_devices(solution_size);
			//pre-calculate the halton numbers sequene			
			for (size_t halton_iter = 0; halton_iter < halton_count; halton_iter++) {
				TSolution tmp;
				tmp.resize(Eigen::NoChange, solution_size);

				// calculate the shift to halton_bases array
				for (size_t i = 0; i < static_cast<size_t>(solution_size); i++)
					tmp[i] = halton_devices[i].advance();//operator()();

				halton_numbers.push_back(tmp);
			}
		}
		

		while ((progress.current_progress++ < max_iterations) && (progress.cancelled == 0)) {
			const TSolution bounds_difference = working_upper_bound - working_lower_bound;			

								
			//for each halton sequence point
			best_solution = tbb::parallel_reduce(tbb::blocked_range<size_t>(1, halton_count), best_solution,
				[this, &bounds_difference, &working_lower_bound, &working_upper_bound, &progress, &halton_numbers, solution_size, solution_size_bit_combinations](const tbb::blocked_range<size_t> &r, Halton_internal::TCandidate<TSolution> default_solution)->Halton_internal::TCandidate<TSolution> {
				
					TSolution drifting_solution;
					drifting_solution.resize(Eigen::NoChange, solution_size);

					for (size_t halton_iter = r.begin(); halton_iter != r.end(); halton_iter++) {
						auto local_metric = mMetric.Clone();
					
						TSolution candidate_solution = halton_numbers[halton_iter] * bounds_difference + working_lower_bound;

						//Now, we have two almost likely different solutions - best and candidate
						//To simulate evolutionary drift/random mutation, let's try to combine them - a deterministic approach						
						for (size_t combination = 0; combination <= solution_size_bit_combinations; combination++) {
						//for (size_t combination = ; combination <= solution_size_bit_combinations; combination <<=1) {	//faster but worse						
							drifting_solution = candidate_solution;

							size_t bit_mask = 1;
							for (size_t bit_index = 0; bit_index < solution_size; bit_index++) {
								if (bit_mask & combination) drifting_solution[bit_index] = default_solution.solution[bit_index];
								bit_mask <<= 1;
							}
						
							drifting_solution = working_upper_bound.min(working_lower_bound.max(drifting_solution));

							mMetric->Reset();
							const double drifting_fitness = mFitness.Calculate_Fitness(drifting_solution, local_metric);
							if (drifting_fitness < default_solution.fitness) {
								default_solution.fitness = drifting_fitness;
								default_solution.solution = drifting_solution;
								progress.best_metric = drifting_fitness;
							}
						}
					}


					return default_solution;
				},
				[](const Halton_internal::TCandidate<TSolution> &a, const Halton_internal::TCandidate<TSolution> &b) { return a.fitness < b.fitness ? a : b;  });

			//zoom the boundaries, i.e., focus them towards the best known solution
			working_lower_bound = (best_solution.solution - working_lower_bound)*mZoom_Factor + working_lower_bound;
			working_upper_bound = (best_solution.solution - working_upper_bound)*mZoom_Factor + working_upper_bound;

		} //max iterations
	

		return best_solution.solution;
	}
};
