#pragma once

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/Eigen_Buffer_Pool.h"

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


namespace pagmo2 {

	enum class EPagmo_Algo {
		PSO,
		SADE,
		DE1220,
		ABC,
		CMAES,
		xNES,
	};


	
template <typename TSolution, typename TFitness>
class TProblem {
 protected:
	TFitness *mFitness;
	mutable glucose::SMetric mMetric;
	pagmo::vector_double mLower_Bound, mUpper_Bound;	
protected:
	volatile glucose::TSolver_Progress *mProgress;
public:
//	https ://esa.github.io/pagmo2/docs/cpp/problem.html#classpagmo_1_1problem
//	a UDP must also be default, copy and move constructible.

	 TProblem(TFitness &fitness, glucose::SMetric metric, const TSolution &lower_bound, const TSolution &upper_bound, volatile glucose::TSolver_Progress *progress) : mFitness(&fitness), mMetric(metric.Clone()), mProgress(progress) {		 

		auto sol_2_vec = [](const TSolution &sol, const double def_val) {
			pagmo::vector_double result;

			double *begin, *end;
			const size_t sol_size = sol.cols();

			if (sol.get(&begin, &end) == S_OK) {
				result.assign(begin, end);
			}
			else {
				result.insert(result.begin(), sol_size, def_val);
			}

			return result;
		};


		mLower_Bound = sol_2_vec(lower_bound, -std::numeric_limits<double>::max());
		mUpper_Bound = sol_2_vec(upper_bound, std::numeric_limits<double>::max());
	}
			
	TProblem() : 
		mFitness(nullptr) {		//should not never occur, but default ctor is just required
	};

	TProblem(const TProblem &other) : 
		mFitness(other.mFitness), mMetric(other.mMetric.Clone()), mLower_Bound(other.mLower_Bound), mUpper_Bound(other.mUpper_Bound), mProgress(other.mProgress) {
	}

		pagmo::vector_double fitness(const pagmo::vector_double &x) const {

			assert(mFitness);
			if (mMetric->Reset() != S_OK)  return pagmo::vector_double(1, std::numeric_limits<double>::max());

			//Eigen::Map<TSolution> mapped{ Map_Double_To_Eigen<TSolution>(x.data(), x.size()) }; - won't work and do not know why
			TSolution mapped;			
			mapped.resize(Eigen::NoChange, x.size());
			for (size_t i = 0; i < x.size(); i++)											//enforce bounds as pagmo does not do so
				mapped[i] = std::min(mUpper_Bound[i], std::max(x[i], mLower_Bound[i]));//ineffective way, but pagmo forbids TSolution desired memory alignment

					
			
			double fitness = mFitness->Calculate_Fitness(mapped, mMetric);
			if (fitness < mProgress->best_metric) mProgress->best_metric = fitness;
			return pagmo::vector_double(1, fitness);			
		}
		
		std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const {
			return { mLower_Bound, mUpper_Bound };
		}
	};

}

template <typename TSolution, typename TFitness, pagmo2::EPagmo_Algo mAlgo, size_t mGeneration_Count = 1'000, size_t mPopulation_Size = 40>
class CPagmo2 {
protected:
	TFitness &mFitness;
	glucose::SMetric &mMetric;
	const TSolution mLower_Bound;
	const TSolution mUpper_Bound;
public:
	CPagmo2(const TAligned_Solution_Vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :
		mLower_Bound(lower_bound), mUpper_Bound(upper_bound), mFitness(fitness), mMetric(metric) {};

	TSolution Solve(volatile glucose::TSolver_Progress &progress) {
		progress.best_metric = std::numeric_limits<double>::max();

		
		//real_problem.Set_Params();
		pagmo::problem prob(pagmo2::TProblem<TSolution, TFitness>{ mFitness, mMetric, mLower_Bound, mUpper_Bound, &progress });
						
		auto solve_pagmo = [&progress, &prob](auto &solver)->pagmo::vector_double {
			pagmo::algorithm algo{ solver };
			pagmo::island isle{ algo, prob, mPopulation_Size };
			isle.evolve();
			isle.wait_check();

			return isle.get_population().champion_x();
		};


		pagmo::vector_double champion_x (mLower_Bound.cols(), std::numeric_limits<double>::quiet_NaN());

		switch (mAlgo) {
			case pagmo2::EPagmo_Algo::PSO:		{ pagmo::pso algo{ mGeneration_Count };   champion_x = solve_pagmo(algo); } break;
			case pagmo2::EPagmo_Algo::SADE:		{ pagmo::sade algo{ mGeneration_Count }; champion_x = solve_pagmo(algo);} break;				
			case pagmo2::EPagmo_Algo::DE1220:	{ pagmo::de1220 algo{ mGeneration_Count }; champion_x = solve_pagmo(algo); } break;
			case pagmo2::EPagmo_Algo::ABC:		{ pagmo::bee_colony algo{ mGeneration_Count }; champion_x = solve_pagmo(algo); } break;
			case pagmo2::EPagmo_Algo::CMAES:	{ pagmo::cmaes algo{ mGeneration_Count }; champion_x = solve_pagmo(algo); } break;
			case pagmo2::EPagmo_Algo::xNES:		{ pagmo::xnes algo{ mGeneration_Count }; champion_x = solve_pagmo(algo); } break;
		}

		TSolution result;
		result.resize(Eigen::NoChange, champion_x.size());
		result.set(champion_x.data(), champion_x.data() + champion_x.size());
		result = mUpper_Bound.min(mLower_Bound.max(result)); //enforce bounds as pagmo does not do so - pagmo prevents us from doing so permanently in the objective function/object of the TProblem class

		return result;
	}

};