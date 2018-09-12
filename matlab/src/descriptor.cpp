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
