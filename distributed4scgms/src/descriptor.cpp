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

#include "descriptor.h"
#include <scgms/iface/DeviceIface.h>
#include <scgms/lang/dstrings.h>
#include <scgms/utils/descriptor_utils.h>
#include <array>

namespace scgms_distributed_solver
{
    const scgms::TSolver_Descriptor distributed_solver_generic_desc = Describe_Non_Specialized_Solver(
        distributed_solver_generic,
        L"Generic distributed solver"
    );
}


const std::array<scgms::TSolver_Descriptor, 1> solver_descriptions = {
    scgms_distributed_solver::distributed_solver_generic_desc,
};


DLL_EXPORT HRESULT IfaceCalling do_get_solver_descriptors(scgms::TSolver_Descriptor** begin,
                                                          scgms::TSolver_Descriptor** end)
{
    return do_get_descriptors(solver_descriptions, begin, end);
}
