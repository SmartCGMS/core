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

#include <scgms/iface/SolverIface.h>

#include "tpNLOpt.h"


#include "descriptor.h"

#include <functional>


template <nlopt::algorithm algo>
bool Solve_NLOpt(const solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {
	CNLOpt<algo> nlopt{ setup };
	return nlopt.Solve(progress);	
}



using  TSolver_Func = std::function<bool(solver::TSolver_Setup &, solver::TSolver_Progress&)>;

struct TSolver_Info
{
	TSolver_Info() = delete;
	explicit TSolver_Info(const GUID& _id, const TSolver_Func& _fnc)
		: id(_id), func(_fnc) {};

	GUID id = Invalid_GUID;
	TSolver_Func func;
};

const std::array<TSolver_Info, 6> solvers = {	TSolver_Info{nlopt::newuoa_id, Solve_NLOpt<nlopt::LN_NEWUOA>},
												TSolver_Info{nlopt::bobyqa_id, Solve_NLOpt<nlopt::LN_BOBYQA>},
												TSolver_Info{nlopt::simplex_id, Solve_NLOpt<nlopt::LN_NELDERMEAD>},
												TSolver_Info{nlopt::subplex_id, Solve_NLOpt<nlopt::LN_SBPLX>},
												TSolver_Info{nlopt::praxis_id, Solve_NLOpt<nlopt::LN_PRAXIS>},
												TSolver_Info{nlopt::cobyla_id, Solve_NLOpt<nlopt::LN_COBYLA>},
};



DLL_EXPORT HRESULT IfaceCalling do_solve_generic(const GUID *solver_id, solver::TSolver_Setup *setup, solver::TSolver_Progress *progress) {
	
	//instead of traversing an array, we could have used map
	//but we no longe have the desire to use these solvers as the Pathfinder outperforms them
	//hence traversing the array is easy to write and the overhead is neglible compared to the work of a solver
	for (const auto &solver : solvers) {
		if (solver.id == *solver_id)
			try {			
				return solver.func(*setup, *progress) ? S_OK : E_INVALIDARG;
			}
			catch (...) {
				return E_FAIL;
			}
  }

	return E_NOTIMPL;
}