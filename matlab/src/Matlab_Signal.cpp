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

#include "Matlab_Signal.h"
#include "../../../common/lang/dstrings.h"

CMatlab_Signal::CMatlab_Signal(std::shared_ptr<matlab::engine::MATLABEngine>& engine, const std::vector<double> &defaultParameters, const std::wstring& getContinuousLevelsScriptName)
	: mDefaultParameters(defaultParameters), mEngine(engine), mGetContinuousLevelsScriptName(getContinuousLevelsScriptName) {
}

CMatlab_Signal::~CMatlab_Signal() {
}

HRESULT IfaceCalling CMatlab_Signal::Get_Discrete_Levels(double* const times, double* const levels, const size_t count, size_t *filled) const {
	return E_NOTIMPL;
}

HRESULT IfaceCalling CMatlab_Signal::Get_Discrete_Bounds(scgms::TBounds *time_bounds, scgms::TBounds *level_bounds, size_t *level_count) const {
	return E_NOTIMPL;
}

HRESULT IfaceCalling CMatlab_Signal::Update_Levels(const double *times, const double *levels, const size_t count) {
	return E_NOTIMPL;
}

HRESULT IfaceCalling CMatlab_Signal::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	matlab::data::ArrayFactory factory;

	double *pbegin, *pend;
	// no parameters = use default
	if (params == nullptr) {
		pbegin = const_cast<double*>(mDefaultParameters.data());
		pend = const_cast<double*>(mDefaultParameters.data()) + mDefaultParameters.size();
	} else {
		if (params->get(&pbegin, &pend) != S_OK)
			return E_FAIL;
	}

	auto paramsArray = factory.createArray({ 1, (size_t)std::distance(pbegin, pend) }, pbegin, pend);
	auto timesArray = factory.createArray({ 1, count }, times, times + count);
	auto derivationScalar = factory.createScalar(derivation_order);

	mEngine->setVariable(rsMatlab_Variable_Model_Parameters, paramsArray);
	mEngine->setVariable(rsMatlab_Variable_Model_Times, timesArray);
	mEngine->setVariable(rsMatlab_Variable_Model_Derivation, derivationScalar);

	std::u16string str{ mGetContinuousLevelsScriptName.begin(), mGetContinuousLevelsScriptName.end() };
	mEngine->eval(str);

	matlab::data::TypedArray<double> output = mEngine->getVariable(rsMatlab_Variable_Model_Output);

	// copy outputs to levels array
	for (size_t i = 0; i < count; i++)
		levels[i] = output[i];

	return S_OK;
}

HRESULT IfaceCalling CMatlab_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const
{
	double *params = const_cast<double*>(mDefaultParameters.data());
	return parameters->set(params, params + mDefaultParameters.size());
}
