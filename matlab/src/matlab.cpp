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

#include "matlab.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/utils/string_utils.h"

#include "descriptor.h"

#include <set>
#include <chrono>
#include <iostream>
#include <stdexcept>

#undef max

// global instance - gets initialized when library is loaded
CMatlab_Factory gMatlab_Factory{ };

CMatlab_Factory::CMatlab_Factory() {
	Load();
}

CMatlab_Factory::~CMatlab_Factory() {
}

void CMatlab_Factory::Parse_Manifest() {

	auto path = Get_Application_Dir();
	path = Path_Append(path, rsMatlab_Manifest_File);

	CXML_Parser<wchar_t> parser(path);
	// although the manifest is required for matlab connection to work, we don't throw an exception or so
	// this is due to possibility the user won't need Matlab filter at all, and this method is called whenever
	// this dynamic library is loaded
	if (!parser.Is_Valid())
		return;
	// any further fatal error, however, throws an exception - this means the manifest is there, but contains
	// invalid information; so we have to throw an exception to let user know

	// session name set by matlab.engine.shareEngine("desired session name"); on MATLAB side
	mMatlab_Session_Name = parser.Get_Parameter(rsMatlab_Manifest_Session_Name_Path, L"");

	mMatlab_Working_Directory = parser.Get_Parameter(rsMatlab_Manifest_Workdir_Path, L".");

	Parse_Models(parser);
	Parse_Solvers(parser);
}

HRESULT CMatlab_Factory::Create_Signal(const GUID *calc_id, scgms::ITime_Segment *segment, scgms::ISignal** signal) {

	for (const auto& model : mModels) {
		for (size_t i = 0; i < model.second.signalGUIDs.size(); i++) {
			if (model.second.signalGUIDs[i] == *calc_id)
				return Manufacture_Object<CMatlab_Signal>(signal, Matlab(), model.second.paramDefaults, model.second.getContinuousLevelsScriptNames[i]);
		}
	}

	return E_NOTIMPL;
}

HRESULT CMatlab_Factory::Solve(const scgms::TSolver_Setup *setup) {
	if (setup->segment_count == 0) return E_INVALIDARG;
	if (mSolvers.find(setup->solver_id) == mSolvers.end()) return E_NOTIMPL;

	const auto& solverRecord = mSolvers[setup->solver_id];

	try
	{
		auto shared_segments = refcnt::Referenced_To_Vector<scgms::STime_Segment, scgms::ITime_Segment>(setup->segments, setup->segment_count);
		const auto shared_lower = refcnt::make_shared_reference_ext<scgms::SModel_Parameter_Vector, scgms::IModel_Parameter_Vector>(setup->lower_bound, true);
		const auto shared_upper = refcnt::make_shared_reference_ext<scgms::SModel_Parameter_Vector, scgms::IModel_Parameter_Vector>(setup->upper_bound, true);
		auto shared_solved = refcnt::make_shared_reference_ext<scgms::SModel_Parameter_Vector, scgms::IModel_Parameter_Vector>(setup->solved_parameters, true);
		auto shared_hints = refcnt::Referenced_To_Vector<scgms::SModel_Parameter_Vector, scgms::IModel_Parameter_Vector>(setup->solution_hints, setup->hint_count);

		scgms::SModel_Parameter_Vector default_parameters;
		if (shared_hints.empty()) {
			auto signal = shared_segments[0].Get_Signal(setup->calculated_signal_id);
			if (signal) {
				default_parameters = refcnt::Create_Container_shared<double, scgms::SModel_Parameter_Vector>(nullptr, nullptr);
				if (signal->Get_Default_Parameters(default_parameters.get()) == S_OK)
					shared_hints.push_back(default_parameters);
			}
		}

		matlab::data::ArrayFactory factory;

		// 1) convert hints and bounds to matlab vectors/matrix
		std::vector<double> paramsMerged;

		size_t paramCnt = 0;
		double *pbegin, *pend;

		const auto appendParamContents = [&paramCnt](const scgms::SModel_Parameter_Vector& vec, std::vector<double>& target) {
			double *pbegin, *pend;
			if (vec->get(&pbegin, &pend) != S_OK)
				return;

			// we use maximum due to the possibility of dynamic parameters
			paramCnt = std::max(paramCnt, static_cast<size_t>(std::distance(pbegin, pend)));

			for (; pbegin != pend; pbegin++)
				target.push_back(*pbegin);
		};

		for (const auto& hint : shared_hints)
			appendParamContents(hint, paramsMerged);
		if (paramCnt == 0) return E_FAIL;

		auto hintsArray = factory.createArray({ paramCnt, shared_hints.size() }, paramsMerged.begin(), paramsMerged.end());

		if (shared_lower->get(&pbegin, &pend) != S_OK) return E_FAIL;
		auto lowBoundArray = factory.createArray({ 1, (size_t)std::distance(pbegin, pend) }, pbegin, pend);

		if (shared_upper->get(&pbegin, &pend) != S_OK) return E_FAIL;
		auto upBoundArray = factory.createArray({ 1, (size_t)std::distance(pbegin, pend) }, pbegin, pend);

		// 2) get discrete times and values of reference signal from all segments

		std::vector<double> times, values;

		GUID referenceId = Invalid_GUID;
		for (const auto& model : mModels) {
			for (size_t i = 0; i < model.second.referenceGUIDs.size(); i++) {
				if (model.second.signalGUIDs[i] == setup->calculated_signal_id) {
					referenceId = model.second.referenceGUIDs[i];
					break;
				}
			}
		}

		if (referenceId == Invalid_GUID) return E_FAIL;

		for (auto& segment : shared_segments) {
			auto sig = segment.Get_Signal(referenceId);
			size_t cnt, filled;
			if (!sig || sig->Get_Discrete_Bounds(nullptr, nullptr, &cnt) != S_OK) return E_FAIL;
			if (cnt == 0) continue;

			times.resize(times.size() + cnt);
			values.resize(values.size() + cnt);

			if (sig->Get_Discrete_Levels(times.data() + times.size() - cnt, values.data() + values.size() - cnt, cnt, &filled) != S_OK)
				return E_FAIL;

			if (filled < cnt) {
				times.resize(times.size() - (cnt - filled));
				values.resize(values.size() - (cnt - filled));
			}
		}

		auto timesArray = factory.createArray({ 1, times.size() }, times.begin(), times.end());
		auto valuesArray = factory.createArray({ 1, values.size() }, values.begin(), values.end());

		// 3) call matlab solver

		auto& engine = Matlab();

		engine->setVariable(rsMatlab_Variable_Solver_Times, timesArray);
		engine->setVariable(rsMatlab_Variable_Solver_Values, valuesArray);
		engine->setVariable(rsMatlab_Variable_Solver_Hints, hintsArray);
		engine->setVariable(rsMatlab_Variable_Solver_Lowbounds, lowBoundArray);
		engine->setVariable(rsMatlab_Variable_Solver_Upbounds, upBoundArray);

		Eval(solverRecord.solveParametersScript);

		// 4) convert solved parameters to array and fill "solved" field of setup

		matlab::data::TypedArray<double> output = engine->getVariable(rsMatlab_Variable_Solver_Output);

		// resulting parameter count does not always remain the same - since there are models, which could yield additional parameters
		// depending on current state (basically the model changes over time)
		//if (paramCnt != output.getNumberOfElements())

		// TODO: after implementation of fixed/dynamic parameters, there should be code which assigns parameter vectors to their matching targets

		// instead, consider 1 or 0 parameters as error (may be the result of error on Matlab side or degrading to scalar, which may also be a result of error
		if (paramCnt <= 1)
			return E_FAIL;

		const std::vector<double> out{ output.begin(), output.end() };
		shared_solved.set(out);
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

bool CMatlab_Factory::Eval(const std::wstring& what)
{
	try
	{
		std::u16string str{ what.begin(), what.end() };
		Matlab()->eval(str);
	}
	catch (...)
	{
		// we do nothing - Matlab shows the error for us
		return false;
	}

	return true;
}

bool Get_Param(const wchar_t* name, const std::map<std::wstring, std::wstring>& paramMap, std::function<void(const std::wstring&)> callback)
{
	auto sitr = paramMap.find(name);
	if (sitr == paramMap.end())
		return false;

	try {
		callback(sitr->second);
	}
	catch (...) { return false; }

	return true;
}

bool CMatlab_Factory::Parse_Models(CXML_Parser<wchar_t> &parser)
{
	if (!parser.Is_Valid())
		return false;

	auto models = parser.Get_Element(rsMatlab_Manifest_Models_Path);
	if (models.children.empty() || models.children.find(rsMatlab_Manifest_Model_Tag) == models.children.end())
		return false;

	for (auto& modelDesc : models.children[rsMatlab_Manifest_Model_Tag])
	{
		bool ok;
		GUID id = WString_To_GUID(modelDesc.parameters[rsMatlab_Manifest_Id_Parameter], ok);
		if (id == Invalid_GUID || !ok)
			continue;

		auto& target = mModels[id];

		target.id = id;
		target.description = modelDesc.Get_Parameter(rsMatlab_Manifest_Description_Parameter);
		target.dbTableName = modelDesc.Get_Parameter(rsMatlab_Manifest_DB_Table_Parameter);

		const auto storeStrParam = [](const wchar_t* name, const std::map<std::wstring, std::wstring>& paramMap, std::vector<std::wstring>& target) -> bool {
			return Get_Param(name, paramMap, [&target](const std::wstring& s) { target.push_back(s); });
		};

		const auto storeDoubleParam = [](const wchar_t* name, const std::map<std::wstring, std::wstring>& paramMap, std::vector<double>& target) -> bool {
			return Get_Param(name, paramMap, [&target](const std::wstring& s) { target.push_back(std::stod(s)); });
		};

		const auto storeGUIDParam = [&ok](const wchar_t* name, const std::map<std::wstring, std::wstring>& paramMap, std::vector<GUID>& target) -> bool {
			return Get_Param(name, paramMap, [&target, &ok](const std::wstring& s) { target.push_back(WString_To_GUID(s, ok)); });
		};

		auto paramElements = modelDesc.children[rsMatlab_Manifest_Parameters_Tag].at(0); // checked by exception mechanism
		for (const auto& element : paramElements.children[rsMatlab_Manifest_Parameter_Tag])
		{
			bool success = storeStrParam(rsMatlab_Manifest_Name_Parameter, element.parameters, target.paramNames)
				&& storeStrParam(rsMatlab_Manifest_DB_Column_Parameter, element.parameters, target.paramDBColumnNames)
				&& storeDoubleParam(rsMatlab_Manifest_Default_Parameter, element.parameters, target.paramDefaults)
				&& storeDoubleParam(rsMatlab_Manifest_Param_Lowbound, element.parameters, target.paramLowerBounds)
				&& storeDoubleParam(rsMatlab_Manifest_Param_Upbound, element.parameters, target.paramUpperBounds);

			if (!success)
				return false;
		}

		auto signalElements = modelDesc.children[rsMatlab_Manifest_Signals_Tag].at(0); // checked by exception mechanism
		for (const auto& element : signalElements.children[rsMatlab_Manifest_Signal_Tag])
		{
			bool success = storeGUIDParam(rsMatlab_Manifest_Id_Parameter, element.parameters, target.signalGUIDs)
				&& storeStrParam(rsMatlab_Manifest_Name_Parameter, element.parameters, target.signalNames)
				&& storeGUIDParam(rsMatlab_Manifest_Reference_Signal_Id_Param, element.parameters, target.referenceGUIDs)
				&& storeStrParam(rsMatlab_Manifest_Script_Name, element.parameters, target.getContinuousLevelsScriptNames);

			if (!success)
				return false;
		}

		for (size_t i = 0; i < target.paramNames.size(); i++)
		{
			target.paramNames_Proxy.push_back(target.paramNames[i].c_str());
			target.paramDBColumnNames_Proxy.push_back(target.paramDBColumnNames[i].c_str());
			// TODO: allow user to define his own parameter type in model XML file
			target.paramTypes.push_back(scgms::NModel_Parameter_Value::mptDouble);
		}

		for (const auto& itr : target.signalNames)
			target.signalNames_Proxy.push_back(itr.c_str());
	}

	return true;
}

bool CMatlab_Factory::Parse_Solvers(CXML_Parser<wchar_t> &parser)
{
	if (!parser.Is_Valid())
		return false;

	auto solvers = parser.Get_Element(rsMatlab_Manifest_Solvers_Path);
	if (solvers.children.empty() || solvers.children.find(rsMatlab_Manifest_Solver_Tag) == solvers.children.end())
		return false;

	for (auto& solverDesc : solvers.children[rsMatlab_Manifest_Solver_Tag])
	{
		bool ok;
		GUID id = WString_To_GUID(solverDesc.parameters[rsMatlab_Manifest_Id_Parameter], ok);
		if (id == Invalid_GUID || !ok)
			continue;

		auto& target = mSolvers[id];

		target.id = id;
		target.description = solverDesc.Get_Parameter(rsMatlab_Manifest_Description_Parameter);
		target.solveParametersScript = solverDesc.Get_Parameter(rsMatlab_Manifest_Script_Name);

		auto modelItr = solverDesc.children.find(rsMatlab_Manifest_Models_Tag);
		if (modelItr != solverDesc.children.end())
		{
			auto modelElements = modelItr->second.at(0);
			for (auto& element : modelElements.children[rsMatlab_Manifest_Model_Tag])
			{
				GUID id = WString_To_GUID(element.Get_Parameter(rsMatlab_Manifest_Id_Parameter), ok);
				if (id != Invalid_GUID || !ok)
					target.modelsSpecialization.push_back(id);
			}
		}
	}

	return true;
}

std::shared_ptr<matlab::engine::MATLABEngine>& CMatlab_Factory::Matlab()
{
	if (!mEngine)
	{
		if (!mMatlab_Session_Name.empty())
		{
			std::u16string str{ mMatlab_Session_Name.begin(), mMatlab_Session_Name.end() };
			try
			{
				mEngine = matlab::engine::connectMATLAB(str);
			}
			catch (...) {
				// we actually handle all possible scenarios below - mEngine would be null anyways on any failure
			}
		}

		if (!mEngine)
			mEngine = matlab::engine::startMATLAB();

		if (!mEngine)
			throw std::runtime_error("Could not start MATLAB session");

		auto dir = mMatlab_Working_Directory;
		if (!dir.empty())
			Eval(L"cd '" + dir + L"'");

		// maybe here we would add some more MATLAB environment init calls in future? Set some variables, if it's needed, etc.
	}

	return mEngine;
}

bool CMatlab_Factory::Load()
{
	Parse_Manifest();

	// register descriptors to descriptor manager (so they could be supplied in descriptor retrieving calls)
	for (auto& model : mModels)
		add_model_descriptor(model.second);
	for (auto& solver : mSolvers)
		add_solver_descriptor(solver.second);

	return true;
}
