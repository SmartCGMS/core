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

#include "../../../../common/iface/ApproxIface.h"
#include "AvgExpApprox.h"

//Epsilon Types - they cannot be declared with extern to allow using them with switch
constexpr size_t etFixedIterations = 1;		//fixed number of iterations
constexpr size_t etMaxAbsDiff = 2;			//maximum aboslute difference of all pointes
constexpr size_t etApproxRelative = 3;		//for each point, maximum difference < (Interpolated-Approximated)*epsilon for interpolated>=approximated
											//else maximum difference < (-Interpolated+Approximated)*epsilon for interpolated<approximated 


extern const double dfYOffset; //some interpolation requires negative values
				//and it is impossible to compute real ln of a negative number
				//100.0 and lower do not work always 

HRESULT AvgExpApproximateData(scgms::WSignal src, CAvgExpApprox **dst, const TAvgExpApproximationParams& params, const TAvgElementary &avgelementary);