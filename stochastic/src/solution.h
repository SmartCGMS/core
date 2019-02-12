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

#include <Eigen/Dense>
#include <vector>
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