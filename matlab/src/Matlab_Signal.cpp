/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
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

HRESULT IfaceCalling CMatlab_Signal::Get_Discrete_Bounds(glucose::TBounds *bounds, size_t *level_count) const {
	return E_NOTIMPL;
}

HRESULT IfaceCalling CMatlab_Signal::Add_Levels(const double *times, const double *levels, const size_t count) {
	return E_NOTIMPL;
}

HRESULT IfaceCalling CMatlab_Signal::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

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

HRESULT IfaceCalling CMatlab_Signal::Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const
{
	double *params = const_cast<double*>(mDefaultParameters.data());
	return parameters->set(params, params + mDefaultParameters.size());
}
