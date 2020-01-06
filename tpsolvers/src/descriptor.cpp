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
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"
#include <array>

namespace nlopt {
	const scgms::TSolver_Descriptor newuoa_desc = Describe_Non_Specialized_Solver(newuoa_id, dsNewUOA);
	const scgms::TSolver_Descriptor bobyqa_desc = Describe_Non_Specialized_Solver(bobyqa_id, dsBOBYQA);
	const scgms::TSolver_Descriptor simplex_desc = Describe_Non_Specialized_Solver(simplex_id, dsSimplex);
	const scgms::TSolver_Descriptor subplex_desc = Describe_Non_Specialized_Solver(subplex_id, dsSubplex);
	const scgms::TSolver_Descriptor praxis_desc = Describe_Non_Specialized_Solver(praxis_id, dsPraxis);
}

namespace pagmo {
	const scgms::TSolver_Descriptor pso_desc = Describe_Non_Specialized_Solver(pso_id, dsPSO);
	const scgms::TSolver_Descriptor sade_desc = Describe_Non_Specialized_Solver(sade_id, dsSADE);
	const scgms::TSolver_Descriptor de1220_desc = Describe_Non_Specialized_Solver(de1220_id, dsDE1220);
	const scgms::TSolver_Descriptor abc_desc = Describe_Non_Specialized_Solver(abc_id, dsABC);
	const scgms::TSolver_Descriptor cmaes_desc = Describe_Non_Specialized_Solver(cmaes_id, dsCMAES);
	const scgms::TSolver_Descriptor xnes_desc = Describe_Non_Specialized_Solver(xnes_id, dsXNES);
	const scgms::TSolver_Descriptor gpso_desc = Describe_Non_Specialized_Solver(gpso_id, dsGPSO);
	const scgms::TSolver_Descriptor ihs_desc = Describe_Non_Specialized_Solver(ihs_id, dsIHS);
}


namespace ppr {
    /*
    [1] K. Tamura and K. Yasuda. Spiral optimization -a new multipoint search method.
        In 2011 IEEE International Conference on Systems, Man, and Cybernetics,
        pages 1759–1764, Oct 2011.
    [2] Kenichi Tamura and Keiichiro Yasuda.Primary study of spiral dynamics inspired
        optimization.IEEJ Transactions on Electrical and Electronic Engineering,
        6(S1) : S98–S100, 2011.        
        */

    const wchar_t* dsSPO = L"Spiral Optimization";
    const scgms::TSolver_Descriptor spo = Describe_Non_Specialized_Solver(spo_id, dsSPO);
}

const std::array<scgms::TSolver_Descriptor, 14> solver_descriptions = { nlopt::newuoa_desc, nlopt::bobyqa_desc, nlopt::simplex_desc, nlopt::subplex_desc, nlopt::praxis_desc,
																		 pagmo::pso_desc, pagmo::sade_desc, pagmo::de1220_desc, pagmo::abc_desc, pagmo::cmaes_desc, pagmo::xnes_desc,
																		 pagmo::gpso_desc, pagmo::ihs_desc, ppr::spo};


HRESULT IfaceCalling do_get_solver_descriptors(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end) {
	return do_get_descriptors(solver_descriptions, begin, end);
}
