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
#include <vector>

#include <tbb/tbb_allocator.h>


namespace mt_metade {
	const glucose::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsMT_MetaDE);
}

namespace halton_metade {
	const glucose::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsHalton_MetaDE);
}

namespace rnd_metade {
	const glucose::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsRnd_MetaDE);
}


namespace halton_sequence {
	const glucose::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsHalton_Zooming);
}

namespace pathfinder {
	const glucose::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsPathfinder);
	const glucose::TSolver_Descriptor desc_LD_Dir = Describe_Non_Specialized_Solver(id_LD_Dir, dsPathfinder_LD_Directions);
	const glucose::TSolver_Descriptor desc_LD_Pop = Describe_Non_Specialized_Solver(id_LD_Dir, dsPathfinder_LD_Population);
	const glucose::TSolver_Descriptor desc_LD_Dir_Pop = Describe_Non_Specialized_Solver(id_LD_Dir, dsPathfinder_LD_Directions_Population);
}


const std::vector<glucose::TSolver_Descriptor, tbb::tbb_allocator<glucose::TSolver_Descriptor>> solver_descriptions = { mt_metade::desc, halton_sequence::desc, halton_metade::desc, rnd_metade::desc, 
																														pathfinder::desc,  pathfinder::desc_LD_Dir, pathfinder::desc_LD_Pop, pathfinder::desc_LD_Dir_Pop};


HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end) {
	return do_get_descriptors(solver_descriptions, begin, end);
}
