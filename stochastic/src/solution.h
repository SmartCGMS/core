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

#include <Eigen/Dense>
#include <vector>
#include <array>

#include <scgms/iface/SolverIface.h>
#include <scgms/rtl/AlignmentAllocator.h>


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
enum class NFitness_Strategy : size_t {
	Strict_Dominance = 0,                   //solution A must be strictly better than solution B
	Soft_Dominance,                         //A is better if it has more dominating fitnesses than B
	Any_Non_Dominated,                      //A is better if it dominates B on at least one fitness - can be used for parent-child only, not for sorting!!
	Euclidean_Dominance,                    //if A does not strictly dominate B, better solution is chosen by its Euclidean distance from the Origin
	Weighted_Euclidean_Dominance,           //metrics are assigned weights, while the first one has the greatest weight (weights: n, n-1, n-2... 1, where n is the number of objectives)
	Ratio_Dominance,                        //if A nor B is not softly dominant, better solution is chosen by Euclidean distance of A[i]/(A[i]+B[i]) and its complement (1.0-a/sum) ratios from the Origin
	Weighted_Ratio_Dominance,

	//any dominance-based strategy must be less than this element!!!

	Dominance_Count,
	Euclidean_Distance = Dominance_Count,
	Weighted_Euclidean_Distance,
	Max_Reduction,

	count,

	//unused aliases has to go as the last [with optionally, disabled strategies moved here in the source code]
	Master = Euclidean_Dominance           //the master, default strategy used to slect the final solution};
};

//returns true if a is better than b - i.e.; if a dominates b
bool Compare_Solutions(const solver::TFitness & a, const solver::TFitness & b, const size_t objectives_count, const NFitness_Strategy strategy);
