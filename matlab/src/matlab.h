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

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/UILib.h"

#include "MatlabDataArray.hpp"
#include "MatlabEngine.hpp"

#include "descriptor.h"
#include "XMLParser.h"

#include <memory>
#include <thread>

#include "Matlab_Signal.h"

/*
 * User-defined model descriptor structure; note this will be converted to scgms::TModel_Descriptor in order
 * to return proper structure via do_get_model_descriptors call
 */
struct TUser_Defined_Model_Descriptor
{
	GUID id;
	std::wstring description;
	std::wstring dbTableName;
	std::vector<std::wstring> paramNames;
	std::vector<const wchar_t*> paramNames_Proxy; // proxy needed to supply correct wchar_t** pointer to const descriptor
	std::vector<std::wstring> paramDBColumnNames;
	std::vector<const wchar_t*> paramDBColumnNames_Proxy;
	std::vector<std::wstring> signalNames;
	std::vector<const wchar_t*> signalNames_Proxy;
	std::vector<GUID> signalGUIDs;
	std::vector<GUID> referenceGUIDs;
	std::vector<double> paramDefaults;
	std::vector<double> paramLowerBounds;
	std::vector<double> paramUpperBounds;
	std::vector<scgms::NModel_Parameter_Value> paramTypes;

	std::vector<std::wstring> getContinuousLevelsScriptNames;
};

/*
 * User-defined solver descriptor structure; note this will be converted to scgms::TSolver_Descriptor in order
 * to return proper structure via do_get_solver_descriptors call
 */
struct TUser_Defined_Solver_Descriptor
{
	GUID id;
	std::wstring description;
	std::vector<GUID> modelsSpecialization;

	std::wstring solveParametersScript;
};

/*
 * Matlab factory class - loads solvers and models from file and manages signal and solver procedures
 */
class CMatlab_Factory
{
	private:
		// instance of Matlab engine - lazyloaded, use Matlab() method to retrieve it
		std::shared_ptr<matlab::engine::MATLABEngine> mEngine;

		// map of models loaded from XML manifest
		std::map<GUID, TUser_Defined_Model_Descriptor> mModels;
		// map of solvers loaded from XML manifest
		std::map<GUID, TUser_Defined_Solver_Descriptor> mSolvers;

	protected:
		// loaded session name (empty for a new session)
		std::wstring mMatlab_Session_Name;
		// working directory we "cd" into - location of user scripts
		std::wstring mMatlab_Working_Directory;

		// lazyloading method of Matlab engine
		std::shared_ptr<matlab::engine::MATLABEngine>& Matlab();
		// evaluates given expression using internal Matlab engine instance
		bool Eval(const std::wstring& what);

	protected:
		// parses manifest file
		void Parse_Manifest();
		bool Parse_Models(CXML_Parser<wchar_t> &parser);
		bool Parse_Solvers(CXML_Parser<wchar_t> &parser);

		// loads this factory - loads manifest, exports models and solvers, ...
		bool Load();

	public:
		CMatlab_Factory();
		virtual ~CMatlab_Factory();

		virtual HRESULT Create_Signal(const GUID *calc_id, scgms::ITime_Segment *segment, scgms::ISignal** signal);
		virtual HRESULT Solve(const scgms::TSolver_Setup *setup);
};

// global instance; note this is initialized when this library gets loaded
extern CMatlab_Factory gMatlab_Factory;
