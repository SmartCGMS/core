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

#include "descriptor.h"
#include "matlab.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/descriptor_utils.h"

#include <vector>
#include <tbb/tbb_allocator.h>

// this will be filled when the library gets loaded and CMatlab_Factory gets initialized
std::vector<glucose::TModel_Descriptor, tbb::tbb_allocator<glucose::TModel_Descriptor>> model_descriptors;

bool add_model_descriptor(TUser_Defined_Model_Descriptor& descriptor)
{
	glucose::TModel_Descriptor desc = {
		descriptor.id,
		descriptor.description.c_str(),
		descriptor.dbTableName.c_str(),
		descriptor.paramNames.size(),
		descriptor.paramTypes.data(),
		descriptor.paramNames_Proxy.data(),
		descriptor.paramDBColumnNames_Proxy.data(),
		descriptor.paramLowerBounds.data(),
		descriptor.paramDefaults.data(),
		descriptor.paramUpperBounds.data(),
		descriptor.signalNames.size(),
		descriptor.signalGUIDs.data(),
		descriptor.signalNames_Proxy.data(),
		descriptor.referenceGUIDs.data()
	};

	model_descriptors.push_back(desc);

	return true;
}

extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end)
{
	if (model_descriptors.size() == 0)
		return S_FALSE;

	*begin = const_cast<glucose::TModel_Descriptor*>(model_descriptors.data());
	*end = *begin + model_descriptors.size();

	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal)
{
	if ((calc_id == nullptr) || (segment == nullptr))
		return E_INVALIDARG;

	return gMatlab_Factory.Create_Signal(calc_id, segment, signal);
}


// this will be filled when the library gets loaded and CMatlab_Factory gets initialized
std::vector<glucose::TSolver_Descriptor, tbb::tbb_allocator<glucose::TSolver_Descriptor>> solver_descriptors;

bool add_solver_descriptor(TUser_Defined_Solver_Descriptor& descriptor)
{
	glucose::TSolver_Descriptor desc = {
		descriptor.id,
		descriptor.description.data(),

		!descriptor.modelsSpecialization.empty(),
		descriptor.modelsSpecialization.size(),
		descriptor.modelsSpecialization.data()
	};

	solver_descriptors.push_back(desc);

	return true;
}

extern "C" HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end) {

	if (solver_descriptors.size() == 0)
		return S_FALSE;

	*begin = const_cast<glucose::TSolver_Descriptor*>(solver_descriptors.data());
	*end = *begin + solver_descriptors.size();

	return S_OK;
}

extern "C" HRESULT IfaceCalling do_solve_model_parameters(const glucose::TSolver_Setup *setup)
{
	return gMatlab_Factory.Solve(setup);
}
