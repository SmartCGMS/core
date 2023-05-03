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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "sensitivity.h"
#include "../descriptor.h"
#include "../../../../common/rtl/SolverLib.h"

#include <algorithm>

/* Constant ISF */

CConstant_Insulin_Sensitivity_Model::CConstant_Insulin_Sensitivity_Model(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment) {
	//
}

HRESULT IfaceCalling CConstant_Insulin_Sensitivity_Model::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const
{
	const const_isf::TParameters &parameters = scgms::Convert_Parameters<const_isf::TParameters>(params, const_isf::default_parameters);

	// for all input times (reference signal times), output constant value
	// this is generally not true for ISF - may vary with time of the day and excercise

    const double val = derivation_order == scgms::apxNo_Derivation ? parameters.isf : 0.0;
    std::fill(levels, levels + count, val);

	return S_OK;
}

HRESULT IfaceCalling CConstant_Insulin_Sensitivity_Model::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(const_isf::default_parameters);
	return parameters->set(params, params + const_isf::param_count);
}

/* Constant CR */

CConstant_Carb_Ratio_Model::CConstant_Carb_Ratio_Model(scgms::WTime_Segment segment) : CCommon_Calculated_Signal(segment) {
	//
}

HRESULT IfaceCalling CConstant_Carb_Ratio_Model::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const
{
	const const_cr::TParameters &parameters = scgms::Convert_Parameters<const_cr::TParameters>(params, const_cr::default_parameters);

	// for all input times (reference signal times), output constant value
	// this is generally not true for CR - may vary with time of the day and other physiological aspects
	
    const double val = derivation_order == scgms::apxNo_Derivation ? parameters.cr : 0.0;
    std::fill(levels, levels + count, val);

	return S_OK;
}

HRESULT IfaceCalling CConstant_Carb_Ratio_Model::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(const_cr::default_parameters);
	return parameters->set(params, params + const_cr::param_count);
}
