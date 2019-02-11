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

#include "factory.h"

#include "../../../common/iface/SolverIface.h"

#include "NLOpt.h"
#include "pagmo2.h"

#include "descriptor.h"


template <nlopt::algorithm algo>
bool Solve_NLOpt(const solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {
	CNLOpt<algo> nlopt{ setup };
	return nlopt.Solve(progress);
	return S_OK;
}



template <pagmo2::EPagmo_Algo algo>
bool Solve_Pagmo(solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {
	CPagmo2<algo> pagmo{ setup };
	return pagmo.Solve(progress);
	return S_OK;
}




using  TSolver_Func = std::function<bool(solver::TSolver_Setup &, solver::TSolver_Progress&)>;

struct TSolver_Info{
	GUID id;
	TSolver_Func func;
};

const std::array<TSolver_Info, 8> solvers = {	TSolver_Info{newuoa::id, Solve_NLOpt<nlopt::LN_NEWUOA>},
												TSolver_Info{bobyqa::id, Solve_NLOpt<nlopt::LN_BOBYQA>},

												TSolver_Info{pso::id, Solve_Pagmo<pagmo2::EPagmo_Algo::PSO>},
												TSolver_Info{sade::id, Solve_Pagmo<pagmo2::EPagmo_Algo::SADE>},
												TSolver_Info{de1220::id, Solve_Pagmo<pagmo2::EPagmo_Algo::DE1220>},
												TSolver_Info{abc::id, Solve_Pagmo<pagmo2::EPagmo_Algo::ABC>},
												TSolver_Info{cmaes::id, Solve_Pagmo<pagmo2::EPagmo_Algo::CMAES>},
												TSolver_Info{xnes::id, Solve_Pagmo<pagmo2::EPagmo_Algo::xNES>},
};



extern "C" HRESULT IfaceCalling do_solve_generic(const GUID *solver_id, solver::TSolver_Setup *setup, solver::TSolver_Progress *progress) {
	
	//instead of traversing an array, we could have used map
	//but with map, we would have to use TBB allocator, which is useless for us at the moment
	//hence traversing the array is easy to write and the overhead is neglible compared to the work of a solver
	for (const auto &solver : solvers) {
		if (solver.id == *solver_id)
			return solver.func(*setup, *progress) ? S_OK : E_FAIL;
  }

	return E_NOTIMPL;
}