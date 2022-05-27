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

#include "factory.h"
#include "descriptor.h"
#include "solution.h"

#include "HaltonDevice.h"
#include "MetaDE.h"
#include "PSO.h"
#include "Sequential_Brute_Force_Scan.h"
#include "Sequential_Convex_Scan.h"
#include "mutation.h"

template <typename TSolver, typename TUsed_Solution>
HRESULT Solve_By_Class(solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {
	TSolver solver{ setup };
	TUsed_Solution result = solver.Solve(progress);
	std::copy(result.data(), result.data() + result.cols(), setup.solution);
	return progress.cancelled == FALSE ? S_OK : E_ABORT;
}


template <typename TUsed_Solution>
class TMT_MetaDE : public CMetaDE<TUsed_Solution, std::mt19937> {};


using  TSolve = std::function<HRESULT(solver::TSolver_Setup &, solver::TSolver_Progress&)>;



template <typename TUsed_Solution>
class CSolution_Dispatcher {
protected:	
	std::map<const GUID, TSolve, std::less<GUID>> mSolver_Id_Map;
public:
	CSolution_Dispatcher() {

		using TMT_MetaDE = CMetaDE<TUsed_Solution, std::mt19937>;
		mSolver_Id_Map[mt_metade::id] = std::bind(&Solve_By_Class<TMT_MetaDE, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);

		using THalton_MetaDE = CMetaDE<TUsed_Solution, CHalton_Device>; 
		mSolver_Id_Map[halton_metade::id] = std::bind(&Solve_By_Class<THalton_MetaDE, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2); 
		
		using TRandom_MetaDE = CMetaDE<TUsed_Solution, std::random_device>;
		mSolver_Id_Map[rnd_metade::id] = std::bind(&Solve_By_Class<TRandom_MetaDE, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);

		mSolver_Id_Map[sequential_brute_force_scan::id] = std::bind(&Solve_By_Class<CSequential_Brute_Force_Scan<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);
		mSolver_Id_Map[sequential_convex_scan::id] = std::bind(&Solve_By_Class<CSequential_Convex_Scan<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);		

		using TPSO_Halton = CPSO<TUsed_Solution, CHalton_Device, pso::CRandom_Swarm_Generator, pso::CDual_Coefficient_Vector_Velocity_Modifier>;
		mSolver_Id_Map[pso::id] = std::bind(&Solve_By_Class<TPSO_Halton, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);

		using TPSO_pr_Halton = CPSO<TUsed_Solution, CHalton_Device, pso::CDiagonal_Swarm_Generator, pso::CDual_Coefficient_Vector_Velocity_Modifier, true>;
		mSolver_Id_Map[pso::pr_id] = std::bind(&Solve_By_Class<TPSO_pr_Halton, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);

		using THalton_OneToNMutation = COne_To_N_Mutation<TUsed_Solution, CHalton_Device>;
		mSolver_Id_Map[mutation::id] = std::bind(&Solve_By_Class<THalton_OneToNMutation, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);
	}

	HRESULT Solve(const GUID &solver_id, solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {

		const auto iter = mSolver_Id_Map.find(solver_id);
		if (iter != mSolver_Id_Map.end())
			return iter->second(setup, progress);
		else return E_NOTIMPL;
	}
};

	


class CId_Dispatcher {
protected:
	CSolution_Dispatcher<TSolution<2>> mDispatcher_2;
	CSolution_Dispatcher<TSolution<4>> mDispatcher_4;
	CSolution_Dispatcher<TSolution<6>> mDispatcher_6;
	CSolution_Dispatcher<TSolution<8>> mDispatcher_8;
	CSolution_Dispatcher<TSolution<Eigen::Dynamic>> mDispatcher_General;	
public:
	HRESULT do_solve(const GUID &solver_id, solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {

		auto call_the_solver = [&](auto &solver)->HRESULT {
			return solver.Solve(solver_id, setup, progress);
		};

		switch (setup.problem_size) {
			case 2: return call_the_solver(mDispatcher_2);
			case 4: return call_the_solver(mDispatcher_4);
			case 6: return call_the_solver(mDispatcher_6);
			case 8: return call_the_solver(mDispatcher_8);
			default: return call_the_solver(mDispatcher_General);
		}

		return E_FAIL;	//just to keep the compiler happy as we never get here
	}
};


static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_generic(const GUID *solver_id, solver::TSolver_Setup *setup, solver::TSolver_Progress *progress) {
	try {
		return Id_Dispatcher.do_solve(*solver_id, *setup, *progress);
	}
	catch (...) {
		return E_FAIL;
	}
}