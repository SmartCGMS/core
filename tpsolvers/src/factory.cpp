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

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "NLOpt.h"
#include "pagmo2.h"

#include "descriptor.h"
#include "../../../common/solver/fitness.h"
#include "../../../common/solver/solution.h"

template <typename TSolution, nlopt::algorithm algorithm_id>
HRESULT Solve_NLOpt(TShared_Solver_Setup &setup, TAligned_Solution_Vector<TSolution>& hints, const TSolution& lower_bound, const TSolution& upper_bound) {

	using TFitness = CFitness<TSolution>;
	using TNLOpt_Specialized_Solver = CNLOpt<TSolution, TFitness, algorithm_id>;
	TFitness fitness{ setup };
	TNLOpt_Specialized_Solver solver{ hints, lower_bound, upper_bound, fitness, setup.metric };

	TSolution solved = solver.Solve(setup.progress);
	Solution_To_Parameters(solved, setup.solved_parameters);

	return S_OK;
}


#define DSpecialized_Id_Dispatcher template <typename TSolution, typename TFitness = CFitness<TSolution>>\
class CLocal_Id_Dispatcher : public virtual CSpecialized_Id_Dispatcher<TSolution, TFitness> {\
protected:\
	template <nlopt::algorithm algorithm_id>\
	void add_NLOpt(const GUID &id) {\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[id] = std::bind(&Solve_NLOpt<TSolution, algorithm_id>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
	}\
public:\
	CLocal_Id_Dispatcher() {\
		add_NLOpt<nlopt::LN_NEWUOA>(newuoa::id);\
		add_NLOpt<nlopt::LN_BOBYQA>(bobyqa::id);\
\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[pso::id] = std::bind(&Solve_By_Class<CPagmo2<TSolution, TFitness, pagmo2::EPagmo_Algo::PSO>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[sade::id] = std::bind(&Solve_By_Class<CPagmo2<TSolution, TFitness, pagmo2::EPagmo_Algo::SADE>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[de1220::id] = std::bind(&Solve_By_Class<CPagmo2<TSolution, TFitness, pagmo2::EPagmo_Algo::DE1220>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[abc::id] = std::bind(&Solve_By_Class<CPagmo2<TSolution, TFitness, pagmo2::EPagmo_Algo::ABC>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[cmaes::id] = std::bind(&Solve_By_Class<CPagmo2<TSolution, TFitness, pagmo2::EPagmo_Algo::CMAES>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[xnes::id] = std::bind(&Solve_By_Class<CPagmo2<TSolution, TFitness, pagmo2::EPagmo_Algo::xNES>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
	}\
};

#include "../../../common/solver/dispatcher.h"


static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_model_parameters(const glucose::TSolver_Setup *setup) {
	return Id_Dispatcher.do_solve_model_parameters(setup);
}