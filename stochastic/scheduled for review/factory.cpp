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
#include "UnconstrainedMetaDE.h"
#include "fast_pathfinder.h"
#include "landscape_pathfinder.h"
#include "Sequential_Brute_Force_Scan.h"
#include "Sequential_Convex_Scan.h"
#include "PSO.h"

template <typename TSolver, typename TUsed_Solution>
HRESULT Solve_Pathfinder_Spiral(solver::TSolver_Setup& setup, solver::TSolver_Progress& progress) {
	solver::TSolver_Setup spiral_setup{
		setup.problem_size,
		setup.lower_bound, setup.upper_bound,
		setup.hints, setup.hint_count,
		setup.solution,
		setup.data, setup.objective,
		0,
		setup.population_size,
		setup.tolerance
	};
		
	TSolver solver{ spiral_setup };
	TUsed_Solution result = solver.Solve(progress);
	std::copy(result.data(), result.data() + result.cols(), setup.solution);
	return progress.cancelled == FALSE ? S_OK : E_ABORT;
}

template <typename TSolver, typename TUsed_Solution>
HRESULT Solve_By_Class(solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {
	TSolver solver{ setup };
	TUsed_Solution result = solver.Solve(progress);
	std::copy(result.data(), result.data() + result.cols(), setup.solution);
	return progress.cancelled == FALSE ? S_OK : E_ABORT;
}

template <typename TSolver, typename TUsed_Solution>
HRESULT Eval_Pathfinder_Angle(solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {
	TUsed_Solution best_solution;

	const double stepping = 0.05;
	double global_fitness = std::numeric_limits<double>::max();
	double best_angle = std::numeric_limits<double>::quiet_NaN();

	double stepped = stepping;//0.0 is illformed solution
	const double pi = atan(1.0)*4.0;
	while (stepped <= 2.0*pi) {
		TSolver solver{ setup, stepped };
		TUsed_Solution local_solution = solver.Solve(progress);
		const double local_fitness = setup.objective(setup.data, local_solution.data());
		if (local_fitness < global_fitness) {
			global_fitness = local_fitness;
			best_solution = local_solution;
			best_angle = stepped;
		}


		stepped += stepping;
	}

	
	std::cout << "best angle: " << best_angle << std::endl;
	std::cout << "best fitness: " << global_fitness << std::endl;

	{
		CFast_Pathfinder<TUsed_Solution> solver{ setup, 0.45 };
		TUsed_Solution local_solution = solver.Solve(progress);
		const double local_fitness = setup.objective(setup.data, local_solution.data());
		std::cout << "0.45 fitness: " << local_fitness << std::endl;
	}

	

	std::copy(best_solution.data(), best_solution.data() + best_solution.cols(), setup.solution);
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

		//mSolver_Id_Map[pathfinder::id] = std::bind(&Eval_Pathfinder_Angle<TUsed_Solution>, std::placeholders::_1, std::placeholders::_2); -- diagnostic
		mSolver_Id_Map[pathfinder::id_fast] = std::bind(&Solve_By_Class<CFast_Pathfinder<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);
		//mSolver_Id_Map[pathfinder::id_fast] = std::bind(&Eval_Pathfinder_Angle<CFast_Pathfinder<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);  -- diagnostic

		mSolver_Id_Map[pathfinder::id_spiral] = std::bind(&Solve_Pathfinder_Spiral<CFast_Pathfinder<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);
		mSolver_Id_Map[pathfinder::id_landscape] = std::bind(&Solve_By_Class<CLandscape_Pathfinder<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);
		
		mSolver_Id_Map[sequential_brute_force_scan::id] = std::bind(&Solve_By_Class<CSequential_Brute_Force_Scan<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);
		mSolver_Id_Map[sequential_convex_scan::id] = std::bind(&Solve_By_Class<CSequential_Convex_Scan<TUsed_Solution>, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);

		using TPSO_Halton = CPSO<TUsed_Solution, CHalton_Device, pso::CRandom_Swarm_Generator, pso::CDual_Coefficient_Vector_Velocity_Modifier>;									
			//using TPSO_MT_DiagInit_SCV = CPSO<TUsed_Solution, std::mt19937, pso::CDiagonal_Swarm_Generator, pso::CSingle_Coefficient_Vector_Velocity_Modifier>;					
			//using TPSO_RND_CrossInit_DCV = CPSO<TUsed_Solution, std::random_device, pso::CCross_Diagonal_Swarm_Generator, pso::CDual_Coefficient_Vector_Velocity_Modifier>;		
		mSolver_Id_Map[pso::id] = std::bind(&Solve_By_Class<TPSO_Halton, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);

		using TPSO_pr_Halton = CPSO<TUsed_Solution, CHalton_Device, pso::CDiagonal_Swarm_Generator, pso::CDual_Coefficient_Vector_Velocity_Modifier, true>;
		mSolver_Id_Map[pso::pr_id] = std::bind(&Solve_By_Class<TPSO_pr_Halton, TUsed_Solution>, std::placeholders::_1, std::placeholders::_2);
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