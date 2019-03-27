#pragma once

#include "../../../common/rtl/SolverLib.h"

#define PAGMO_WITH_EIGEN3

#include <pagmo/types.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/island.hpp>

#include <pagmo/algorithms/pso.hpp>
#include <pagmo/algorithms/sade.hpp>
#include <pagmo/algorithms/de1220.hpp>
#include <pagmo/algorithms/bee_colony.hpp>
#include <pagmo/algorithms/cmaes.hpp>
#include <pagmo/algorithms/xnes.hpp>
#include <pagmo/algorithms/pso_gen.hpp>
#include <pagmo/algorithms/ihs.hpp>

namespace pagmo2 {

	enum class NPagmo_Algo {
		PSO,
		SADE,
		DE1220,
		ABC,
		CMAES,
		xNES,
		GPSO,
		IHS		
	};

	
	class TProblem {
	 protected:
		 const solver::TSolver_Setup &mSetup;
	protected:
		solver::TSolver_Progress mVoid_Progress = solver::Null_Solver_Progress;	//just because Pagmo insists on default ctor
		solver::TSolver_Progress &mProgress;

		mutable pagmo::vector_double mBounded_Solution;	
	public:
	//	https ://esa.github.io/pagmo2/docs/cpp/problem.html#classpagmo_1_1problem
	//	a UDP must also be default, copy and move constructible.

		TProblem(const solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) : mSetup(setup), mProgress(progress) { 
			mBounded_Solution.resize(setup.problem_size); 
		}
			
		TProblem() : 
			mSetup(solver::Default_Solver_Setup), mProgress(mVoid_Progress) {		//should not never occur, but default ctor is just required		
		};

		TProblem(const TProblem &other) : 
			mSetup(other.mSetup), mProgress(other.mProgress) {	
			mBounded_Solution.resize(mSetup.problem_size);
		}

		pagmo::vector_double fitness(const pagmo::vector_double &x) const {

			assert(mSetup.problem_size>0);
			
			//because of https://github.com/esa/pagmo2/issues/143
			//we enforce bounds on our own solution vector as the fitness method won't be called in parallel


			for (size_t i = 0; i < x.size(); i++)											//enforce bounds as pagmo does not do so
				mBounded_Solution[i] = std::min(mSetup.upper_bound[i], std::max(x[i], mSetup.lower_bound[i]));


			const double fitness = mSetup.objective(mSetup.data, mBounded_Solution.data());
			if (fitness < mProgress.best_metric) mProgress.best_metric = fitness;
			return pagmo::vector_double(1, fitness);			
		}
		
		std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const {
			pagmo::vector_double lb{ mSetup.lower_bound,  mSetup.lower_bound + mSetup.problem_size};
			pagmo::vector_double ub{ mSetup.upper_bound, mSetup.upper_bound + mSetup.problem_size};
			return { lb, ub };
		}
	};

}


template <pagmo2::NPagmo_Algo mAlgo>
class CPagmo2 {
protected:
	solver::TSolver_Setup mSetup;		
public:
	CPagmo2(const solver::TSolver_Setup &setup) : mSetup(solver::Check_Default_Parameters(setup, 100'000, 100)) {
	};

	bool Solve(solver::TSolver_Progress &progress) {
		pagmo::vector_double champion_x(mSetup.problem_size, std::numeric_limits<double>::quiet_NaN());
		bool succeded = false;

		auto ps = [this, &progress, &champion_x, &succeded](auto solver) {			

			pagmo::problem prob(pagmo2::TProblem{ mSetup, progress });

			pagmo::algorithm algo{ solver };
			pagmo::island isle{ algo, prob, mSetup.population_size };
			isle.evolve();
			isle.wait_check();

			succeded = isle.status() == pagmo::evolve_status::idle;

			champion_x= isle.get_population().champion_x();

		};

		switch (mAlgo) {
			case pagmo2::NPagmo_Algo::PSO:		ps(pagmo::pso{ static_cast<unsigned int>(mSetup.max_generations) }); break;
			case pagmo2::NPagmo_Algo::SADE:		ps(pagmo::sade{ static_cast<unsigned int>(mSetup.max_generations), 2, 1, mSetup.tolerance }); break;
			case pagmo2::NPagmo_Algo::DE1220:	ps(pagmo::de1220{ static_cast<unsigned int>(mSetup.max_generations), pagmo::de1220_statics<void>::allowed_variants, 1, mSetup.tolerance }); break;
			case pagmo2::NPagmo_Algo::ABC:		ps(pagmo::bee_colony{ static_cast<unsigned int>(mSetup.max_generations) });  break;
			case pagmo2::NPagmo_Algo::CMAES:	ps(pagmo::cmaes{ static_cast<unsigned int>(mSetup.max_generations), -1.0, -1.0, -1.0, -1.0, 0.5, mSetup.tolerance });  break;
			case pagmo2::NPagmo_Algo::xNES:		ps(pagmo::xnes{ static_cast<unsigned int>(mSetup.max_generations), -1.0, -1.0, -1.0, -1.0, mSetup.tolerance }); break;
			case pagmo2::NPagmo_Algo::GPSO:		ps(pagmo::pso_gen{ static_cast<unsigned int>(mSetup.max_generations) }); break;
			case pagmo2::NPagmo_Algo::IHS:		ps(pagmo::ihs{ static_cast<unsigned int>(mSetup.max_generations) }); break;			
		}
		
		for (size_t i = 0; i < champion_x.size(); i++)		//enforce bounds as pagmo does not do so - pagmo prevents us from doing so permanently in the objective function/object of the TProblem class
			mSetup.solution[i] = std::min(mSetup.upper_bound[i], std::max(champion_x[i], mSetup.lower_bound[i]));
		
		return succeded;
	}

};