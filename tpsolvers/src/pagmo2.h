#pragma once

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/Eigen_Buffer_Pool.h"


/*#include <pagmo/external/cereal/cereal.hpp>
#include <pagmo/external/cereal/types/polymorphic.hpp>
#include <pagmo/external/cereal/details/polymorphic_impl.hpp>
*/

#include <pagmo/types.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/island.hpp>
#include <pagmo/algorithms/sade.hpp>
#include <pagmo/algorithms/pso.hpp>

namespace pagmo2 {

	enum class EPagmo_Algo {
		SADE,
		PSO
	};


	template <typename TSolution, typename TFitness>
	class TProblem {
	protected:
		TFitness &mFitness;
		mutable glucose::SMetric mMetric;
		pagmo::vector_double mLower_Bound, mUpper_Bound;
	protected:
		volatile glucose::TSolver_Progress *mProgress;
	public:
		TProblem(TFitness &fitness, glucose::SMetric metric, const TSolution &lower_bound, const TSolution &upper_bound, volatile glucose::TSolver_Progress *progress) : mFitness(fitness), mMetric(metric), mProgress(progress) {

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
		
		pagmo::vector_double fitness(const pagmo::vector_double &x) const {			
	/*		Eigen::Map<TSolution> mapped{ Map_Double_To_Eigen<TSolution>(x.data(), x.size()) };			
			glucose::SMetric metric = mMetric.Clone();
			const double fitness = mFitness.Calculate_Fitness(mapped, metric);
			if (fitness < mProgress->best_metric) mProgress->best_metric = fitness;
			return pagmo::vector_double(1, fitness);
			*/
			return pagmo::vector_double(1, 0);
		}
		
		std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const {
			return { mLower_Bound, mUpper_Bound };
		}
	};
}

template <typename TSolution, typename TFitness, pagmo2::EPagmo_Algo mAlgo, size_t mGeneration_Count = 10'000, size_t mPopulation_Size = 100>
class CPagmo2 {
protected:
	TFitness &mFitness;
	glucose::SMetric &mMetric;
	const TSolution mLower_Bound;
	const TSolution mUpper_Bound;
protected:
	pagmo::vector_double Solve_Pagmo( volatile glucose::TSolver_Progress &progress) {
		
	}
public:
	CPagmo2(const TAligned_Solution_Vector<TSolution> &initial_solutions, const TSolution &lower_bound, const TSolution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :
		mLower_Bound(lower_bound), mUpper_Bound(upper_bound), mFitness(fitness), mMetric(metric) {};

	TSolution Solve(volatile glucose::TSolver_Progress &progress) {
		progress.best_metric = std::numeric_limits<double>::max();
		pagmo::problem prob;// { pagmo2::TProblem<TSolution, TFitness>{mFitness, mMetric, mLower_Bound, mUpper_Bound, &progress } };
		
		auto solve_pagmo = [&progress, &prob](auto &solver)->pagmo::vector_double {
			pagmo::algorithm algo{ solver };
			pagmo::island isle{ algo, prob, mPopulation_Size };
			isle.evolve(mGeneration_Count);
			isle.wait_check();

			return isle.get_population().champion_x();
		};


		pagmo::vector_double champion_x (mLower_Bound.cols(), std::numeric_limits<double>::quiet_NaN());

		switch (mAlgo) {
			case pagmo2::EPagmo_Algo::SADE: 
				{  
					pagmo::sade sade{ mGeneration_Count };
					champion_x = solve_pagmo(sade);
				}
				break;

				
			case pagmo2::EPagmo_Algo::PSO:	
				{	pagmo::pso pso{ mGeneration_Count };
					champion_x = solve_pagmo(pso);
				}
				break;
		}

	
		//Eigen::Map<TSolution> mapped{ Map_Double_To_Eigen<TSolution>(champion_x.data(), champion_x.size()) };

		//return mapped;
		return mLower_Bound;
	}

};