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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include <Eigen/Dense>
#include <vector>
#include <array>

#include "../../../common/iface/SolverIface.h"
#include "../../../common/rtl/AlignmentAllocator.h"


template <int n>
using TSolution = Eigen::Array<double, 1, n, Eigen::RowMajor>;

template <typename TUsed_Solution>
using TAligned_Solution_Vector = std::vector<TUsed_Solution, AlignmentAllocator<TUsed_Solution>>;

template <typename TUsed_Solution>
TUsed_Solution Vector_2_Solution(const double *vector, const size_t n) {
	//to avoid all the complications with Eigein-class deriving, let's do this as a standalone function
	TUsed_Solution result;
	result.resize(Eigen::NoChange, n);
	std::copy(vector, vector + n, result.data());	
	return result;
}


	
		//Beware, origin is the ultimate, best fitness - negative fitness are not allowed by all strategies
		//Dominance based strategies must go first!
enum class NFitness_Strategy : size_t { Strict_Dominance = 0,					//solution A must be strictly better than solution B
									    Soft_Dominance,							//A is better if it has more dominating fitnesses than B	
										Euclidean_Dominance,					//if A nor B is not softly dominant, better solution is chosen by its Euclidean distance from the Origin
										Weighted_Euclidean_Dominance,			//metrics are assigned weights, while the first one has the greatest weight (weights: n, n-1, n-2... 1, where n is the number of objectives)
										Ratio_Dominance,						//if A nor B is not softly dominant, better solution is chosen by Euclidean distance of A[i]/(A[i]+B[i]) and its complement (1.0-a/sum) ratios from the Origin
										Weighted_Ratio_Dominance,				
		
										//any dominance-based strategy must be less than this element!!!
										Dominance_Count,						
										Euclidean_Distance = Dominance_Count,
										Weighted_Euclidean_Distance,

										//by this we ended multiple-objective strategies
										MO_Count,
										
										//single objectives must go last
										Objective_0 = MO_Count,							//just a single objective is better
										Objective_1,
										Objective_2,
										Objective_3, 
										Objective_4,
										Objective_5,
										Objective_6,
										Objective_7,
										Objective_8,
										Objective_9,
										count,

										//unused aliases has to go as the last [with optionally, disabled strategies moved here in the source code]
										Master = Euclidean_Dominance};			//the master, default strategy used to slect the final solution};

bool Compare_Solutions(const solver::TFitness & a, const solver::TFitness & b, const size_t objectives_count, const NFitness_Strategy strategy);
		//returns true if a is better than b - i.e.; if a dominates b