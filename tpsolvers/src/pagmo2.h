#pragma once

#include "../../../common/rtl/SolverLib.h"
#include "tpNLOpt.h"

#include <algorithm>
#include <mutex>

#if !defined(PAGMO_WITH_EIGEN3)
	#define PAGMO_WITH_EIGEN3
#endif

#include <pagmo/types.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/island.hpp>
#include <pagmo/archipelago.hpp>
#include <pagmo/utils/multi_objective.hpp>

#include <pagmo/algorithms/pso.hpp>
#include <pagmo/algorithms/sade.hpp>
#include <pagmo/algorithms/de1220.hpp>
#include <pagmo/algorithms/bee_colony.hpp>
#include <pagmo/algorithms/cmaes.hpp>
#include <pagmo/algorithms/xnes.hpp>
#include <pagmo/algorithms/pso_gen.hpp>
#include <pagmo/algorithms/ihs.hpp>
#include <pagmo/algorithms/nsga2.hpp>
#include <pagmo/algorithms/moead.hpp>
#include <pagmo/algorithms/maco.hpp>
#include <pagmo/algorithms/nspso.hpp>

namespace pagmo2 {

	enum class NPagmo_Algo {
		PSO,
		SADE,
		DE1220,
		ABC,
		CMAES,
		xNES,
		GPSO,
		IHS,
		NSGA2,
		MOEAD,
		MACO,
		NSPSO
	};


std::mutex guard;
std::vector<double> best_solution;
double best_distance = std::numeric_limits<double>::max();

	
	class CRemap {
	protected:
		pagmo::vector_double mRemapped_solution, mRemapped_Lower, mRemapped_Upper;	//must be initialized to the default solution as only the remapped elements will be replaced	
		std::vector<size_t> mDimension_Remap;
		const solver::TSolver_Setup mSetup;
	public:
		CRemap(const solver::TSolver_Setup& setup) : mSetup(setup) {


			for (size_t i = 0; i < mSetup.problem_size; i++) {
				if (mSetup.lower_bound[i] != mSetup.upper_bound[i]) {
					mDimension_Remap.push_back(i);
					mRemapped_Lower.push_back(mSetup.lower_bound[i]);
					mRemapped_Upper.push_back(mSetup.upper_bound[i]);
				}
			}
		}


			std::vector<double> Expand_Solution(const pagmo::vector_double & x) const {
				std::vector<double> solution{ mSetup.lower_bound, mSetup.lower_bound + mSetup.problem_size };
				for (size_t i = 0; i < x.size(); i++) {
					solution[mDimension_Remap[i]] = std::min(mRemapped_Upper[i], std::max(x[i], mRemapped_Lower[i]));
				}

				return solution;
			}

			pagmo::vector_double Reduce_Solution(const double* solution) {
				pagmo::vector_double result;

				for (size_t i = 0; i < mDimension_Remap.size(); i++)
					result.push_back(solution[mDimension_Remap[i]]);

				return result;
			}

			std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const {			
				return { mRemapped_Lower, mRemapped_Upper };
			}

			size_t problem_size() const {
				return mDimension_Remap.size();
			}
	};


	class TProblem {
	protected:
		 const solver::TSolver_Setup &mSetup;
		 const CRemap mRemap;
	protected:
		solver::TSolver_Progress mVoid_Progress = solver::Null_Solver_Progress;	//just because Pagmo insists on default ctor
		solver::TSolver_Progress &mProgress;
	public:
	//	https ://esa.github.io/pagmo2/docs/cpp/problem.html#classpagmo_1_1problem
	//	a UDP must also be default, copy and move constructible.

		TProblem(const solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) : mSetup(setup), mProgress(progress), mRemap(setup) { 
		}
			
		TProblem() : 
			mSetup(solver::Default_Solver_Setup), mProgress(mVoid_Progress), mRemap(solver::Default_Solver_Setup) {		//should not never occur, but default ctor is just required					
		};

		TProblem(const TProblem &other) : 
			mSetup(other.mSetup), mProgress(other.mProgress), mRemap(other.mRemap)  {

		}

		//TODO: override thise to enable batch fitness computation
		//vector_double batch_fitness(const vector_double&) const;  - fitness should just call this one
		//bool has_batch_fitness() const;

		pagmo::vector_double fitness(const pagmo::vector_double &x) const {

			assert(mSetup.problem_size>0);
			
			const auto solution = mRemap.Expand_Solution(x);

			pagmo::vector_double result(solver::Maximum_Objectives_Count, std::numeric_limits<double>::quiet_NaN());	//mSetup.objective may expect all the solver::TFitness
			mSetup.objective(mSetup.data, 1, solution.data(), result.data());	//should it fail, result should remain set to nan 
			

			const double local_distance = Solution_Distance(mSetup.objectives_count , result);
			if (local_distance<pagmo2::best_distance) {
				std::unique_lock<std::mutex> scoped_guard {pagmo2::guard};
					if (local_distance<pagmo2::best_distance) {
pagmo2::best_solution.assign(solution.begin(), solution.end());
pagmo2::best_distance = local_distance;
std::cout << "\nBest Distance: " << local_distance << "; ";

for (size_t i = 0; i < mSetup.objectives_count; i++) {
	mProgress.best_metric[i] = result[i];
	if (i > 0)
		std::cout << "; ";
	std::cout << i << ":" << result[i];
}

std::cout << std::endl;
}	
			}


//			if (result[0] < mProgress.best_metric[0]) //mProgress.best_metric[0] = result[0];



			//if (mSetup.objective(mSetup.data, 1, mBounded_Solution.data(), result.data()) != TRUE)
			//				return pagmo::vector_double{ solver::Nan_Fitness.begin(), solver::Nan_Fitness.end() };

			//if (fitness[0] < mProgress.best_metric[0]) mProgress.best_metric[0] = fitness[0];
			//return pagmo::vector_double(1, fitness[0]);

			result.resize(mSetup.objectives_count);	//pagmo expects actual objective count
			return result;
		}
		
		std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const {
			return mRemap.get_bounds();
		}

		pagmo::vector_double::size_type get_nobj() const {
			return mSetup.objectives_count;
		}
	};

}


template <pagmo2::NPagmo_Algo mAlgo>
class CPagmo2 {
protected:
	solver::TSolver_Setup mSetup;
protected:
	pagmo2::CRemap mRemap;
public:
	CPagmo2(const solver::TSolver_Setup &setup) : mSetup(solver::Check_Default_Parameters(setup, 100'000, 100)), mRemap(setup) {		

		pagmo2::best_distance = Solution_Distance(mSetup.objectives_count, mSetup.hints[0]);
		pagmo2::best_solution = std::move(std::vector<double>{mSetup.hints[0], mSetup.hints[0]+mSetup.problem_size});

	};

	bool Solve(solver::TSolver_Progress &progress) {
		pagmo::vector_double champion_x(mRemap.problem_size(), std::numeric_limits<double>::quiet_NaN());
		bool succeded = false;

		auto ps = [this, &progress, &champion_x, &succeded](auto solver) {			

			auto my_problem = pagmo2::TProblem{ mSetup, progress };

			pagmo::problem prob{ std::move(my_problem) };

			pagmo::algorithm algo{ solver };
			algo.set_verbosity(10/*0*/);
			

			//isle population must be greater at least and divisable by 4
			size_t isle_population = std::max(static_cast<size_t>(8), mSetup.population_size / std::thread::hardware_concurrency());
			isle_population += isle_population % 4;

			pagmo::archipelago archi{ std::thread::hardware_concurrency(), algo, prob, isle_population };
			///pagmo::archipelago archi{ 2, algo, prob, 8 };
			//pagmo::archipelago archi{ 1, algo, prob, mSetup.population_size };

			if (mSetup.hint_count > 0) {
				//insert the hints into the population
				auto population = archi[0].get_population();
				const size_t pop_size = population.size();
				for (size_t i = 0; i < std::min(mSetup.hint_count, pop_size); i++) {
					population.set_x(i, mRemap.Reduce_Solution(mSetup.hints[i]));
				}
				archi[0].set_population(population);
			}
			
			archi.evolve();
			archi.wait_check();

			succeded = archi.status() == pagmo::evolve_status::idle;
/*

			//with a single-objective, we could just call get_champion_x
			//but this code works with both single- and multi-objective
			std::vector<pagmo::vector_double> all_solutions, all_fitnesses;
			for (size_t i = 0; i < archi.size(); i++) {
				auto population = archi[i].get_population();
				auto solutions = population.get_x();
				all_solutions.insert(all_solutions.end(), solutions.begin(), solutions.end());
				
				auto fitnesses = population.get_f();
				all_fitnesses.insert(all_fitnesses.end(), fitnesses.begin(), fitnesses.end());
			}
			
			std::vector<size_t> indexes(all_solutions.size());
			std::iota(indexes.begin(), indexes.end(), 0);

			if (mSetup.objectives_count > 1) {
				std::vector<double> scalar_fitnesses;
				for (size_t i = 0; i < all_fitnesses.size(); i++) {
					double accu = 0.0;
					for (size_t j = 0; j < mSetup.objectives_count; j++) {
						const double e = all_fitnesses[i][j];
						accu += e * e;
					}
					scalar_fitnesses.push_back(accu);
				}

				std::sort(indexes.begin(), indexes.end(), [&](const size_t a, const size_t b) {return scalar_fitnesses[indexes[a]] < scalar_fitnesses[indexes[b]]; });

			} else
				std::sort(indexes.begin(), indexes.end(), [&](const size_t a, const size_t b) {return all_fitnesses[indexes[a]][0] < all_fitnesses[indexes[b]][0]; });
	
			champion_x = mRemap.Expand_Solution(all_solutions[indexes[0]]);
*/
champion_x.assign(pagmo2::best_solution.begin(), pagmo2::best_solution.end());
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
			case pagmo2::NPagmo_Algo::NSGA2:	ps(pagmo::nsga2{ static_cast<unsigned int>(mSetup.max_generations) }); break;
			case pagmo2::NPagmo_Algo::MOEAD:	ps(pagmo::moead{ static_cast<unsigned int>(mSetup.max_generations) }); break;
			case pagmo2::NPagmo_Algo::MACO:		ps(pagmo::maco{ static_cast<unsigned int>(mSetup.max_generations) }); break;
			case pagmo2::NPagmo_Algo::NSPSO:	ps(pagmo::nspso{ static_cast<unsigned int>(mSetup.max_generations) }); break;
		}
		
		std::copy(champion_x.begin(), champion_x.end(), mSetup.solution);
		
		return succeded;
	}

};