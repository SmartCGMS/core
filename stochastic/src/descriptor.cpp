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

#include "descriptor.h"
#include <scgms/iface/DeviceIface.h>
#include <scgms/lang/dstrings.h>
#include <scgms/utils/descriptor_utils.h>
#include <vector>

namespace mt_metade {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsMT_MetaDE);
}

namespace halton_metade {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsHalton_MetaDE);
}

namespace rnd_metade {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsRnd_MetaDE);
}

namespace xss_metade {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, L"Xor-Shift* MetaDE");
}

namespace pso {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsPSO_Halton);
	const scgms::TSolver_Descriptor desc_r = Describe_Non_Specialized_Solver(pr_id, L"Halton pr-PSO");
}

namespace sequential_brute_force_scan {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsSequential_Brute_Force_Scan);
}

namespace sequential_convex_scan {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsSequential_Convex_Scan);
}

namespace mutation {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, L"1-to-N mutation");
}

namespace rumoropt {
	const scgms::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, L"RumorOpt");
}

const std::array<scgms::TSolver_Descriptor, 10> solver_descriptions = {
	mt_metade::desc, halton_metade::desc, rnd_metade::desc, xss_metade::desc,
	pso::desc, pso::desc_r,
	sequential_brute_force_scan::desc, sequential_convex_scan::desc,
	mutation::desc, rumoropt::desc,
};


DLL_EXPORT HRESULT IfaceCalling do_get_solver_descriptors(scgms::TSolver_Descriptor **begin, scgms::TSolver_Descriptor **end) {
	return do_get_descriptors(solver_descriptions, begin, end);
}
