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

#include "diabetes_grid.h"

/// <summary>
/// Clarke Error Grid
/// </summary>
/// <remarks>
/// The Clarke error grid approach is used to assess the clinical
/// significance of differences between the glucose measurement technique
/// under test and the venous blood glucose reference measurements.The
/// method uses a Cartesian diagram, in which the values predicted by the
/// technique under test are displayed on the y - axis, whereas the values
/// received from the reference method are displayed on the x - axis.The
/// diagonal represents the perfect agreement between the two, whereas the
/// points below and above the line indicate, respectively, overestimation
/// and underestimation of the actual values.Zone A(acceptable) represents
/// the glucose values that deviate from the reference values by 20 % or are
/// in the hypoglycemic range(<70 mg / dl), when the reference is also within
/// the hypoglycemic range.The values within this range are clinically exact
/// and are thus characterized by correct clinical treatment.Zone B(benign
/// errors) is located above and below zone A; this zone represents those
/// values that deviate from the reference values, which are incremented by
/// 20 % .The values that fall within zones A and B are clinically acceptable,
/// See also:
/// A. Maran et al. "Continuous Subcutaneous Glucose Monitoring in Diabetic 
/// Patients" Diabetes Care, Volume 25, Number 2, February 2002
/// </remarks>


extern const TError_Grid Clarke_Error_Grid;