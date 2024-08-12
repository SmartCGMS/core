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

#include "Constant_Model.h"
#include "../descriptor.h"

#include <scgms/rtl/SolverLib.h>

#include <cmath>

CConstant_Model::CConstant_Model(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment) {
	//
}

HRESULT IfaceCalling CConstant_Model::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	const constant_model::TParameters &parameters = scgms::Convert_Parameters<constant_model::TParameters>(params, constant_model::default_parameters);

	// for all input times (reference signal times), output constant value
	// this comes in handy for e.g. measuring quality of regulation - metrics then can tell us, how good the regulation actually was,
	// how many values were in target range, etc.

	const double val = derivation_order == scgms::apxNo_Derivation ? parameters.c : 0.0;
	std::fill(levels, levels + count, val);

	return S_OK;
}

HRESULT IfaceCalling CConstant_Model::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(constant_model::default_parameters);
	return parameters->set(params, params + constant_model::param_count);
}
