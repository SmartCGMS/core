/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#pragma once



#include <nlopt.hpp>
#include "solution.h"
#include "NullMethod.h"

#include "..\..\..\common\DebugHelper.h"

#undef min

namespace nlopt_tx {
	constexpr bool print_statistics = false;
	std::atomic<size_t> eval_counter;
}

/*

   Non-linear optimization may give better result, if we break appart nested equations such as
   the left side of diffusion model and its phi(t). Hence we need to instantiate
   top-level solver for phi(t) parameters and then launch bottom-level solver
   for the linear paramters.
  
*/


template <typename TResult_Solution, typename TTop_Solution, typename TBottom_Solution, typename TFitness>
struct TNLOpt_Objective_Function_Data {

	TResult_Solution best_solution;
	double best_solution_fitness;

	TFitness &fitness;
	TBottom_Solution& lower_bottom_bound, upper_bottom_bound;
	std::vector<TBottom_Solution> initial_bottom_solution;

	TTop_Solution &default_top_solution;
	std::vector<size_t> &dimension_remap;

	glucose::SMetric metric; //don't make this a reference!	

	volatile glucose::TSolver_Progress *progress;
	nlopt::opt &options;
};

template <typename TFitness, typename TTop_Solution, typename TBottom_Solution>
class CNLOpt_Fitness_Proxy {
protected:
	TFitness &mOriginal_Fitness;
	TTop_Solution mTop_Solution;
public:
	CNLOpt_Fitness_Proxy(TFitness &original_fitness, const TTop_Solution &top_solution) :
		mOriginal_Fitness(original_fitness), mTop_Solution(top_solution) {};

	double Calculate_Fitness(const TBottom_Solution &bottom_solution, glucose::SMetric &metric) const {

		return mOriginal_Fitness.Calculate_Fitness(mTop_Solution.Compose(bottom_solution), metric);
	};
};

//let's use use_remap to eliminate the overhead of remapping, when it is not needed - compiler will generate two functions for us, just from a single source code
template <typename TResult_Solution, typename TTop_Solution, typename TBottom_Solution, typename TBottom_Solver, typename TFitness, bool use_remap>
double NLOpt_Top_Solution_Objective_Function(const std::vector<double> &x, std::vector<double> &grad, void *my_func_data) {
	TNLOpt_Objective_Function_Data<TResult_Solution, TTop_Solution, TBottom_Solution, TFitness> * data = (TNLOpt_Objective_Function_Data<TResult_Solution, TTop_Solution, TBottom_Solution, TFitness>*)my_func_data;

	TTop_Solution top_result;// = TTop_Solution::From_Vector(x);
	if (use_remap) {
		top_result = data->default_top_solution;
		for (size_t i = 0; i < x.size(); i++) {
			top_result[data->dimension_remap[i]] = x[i];
		}
	} else
		top_result.set(x.data(), x.data()+x.size());
	

	CNLOpt_Fitness_Proxy<TFitness, TTop_Solution, TBottom_Solution> fitness_proxy{ data->fitness, top_result };
	TBottom_Solver bottom_solver(data->initial_bottom_solution, data->lower_bottom_bound, data->upper_bottom_bound, fitness_proxy, data->metric);
	glucose::TSolver_Progress tmp_progress = { 0 };
	TBottom_Solution bottom_result = bottom_solver.Solve(tmp_progress);

	TResult_Solution composed_result = top_result.Compose(bottom_result);
	

	double fitness = data->fitness.Calculate_Fitness(composed_result, data->metric);


	if (fitness < data->best_solution_fitness) {
		data->best_solution_fitness = fitness;
		data->best_solution = composed_result;

		data->progress->best_metric = fitness;
	}

	data->progress->current_progress++;	

	if (nlopt_tx::print_statistics)	nlopt_tx::eval_counter++;

	if (data->progress->cancelled) data->options.set_force_stop(true);

	return fitness;
}


template <typename TResult_Solution, typename TFitness, nlopt::algorithm mTop_Algorithm, typename TTop_Solution = TResult_Solution, typename TBottom_Solution = TResult_Solution, typename TBottom_Solver = CNullMethod<TResult_Solution, CNLOpt_Fitness_Proxy<TFitness, TResult_Solution, TResult_Solution>>, size_t max_eval = 0>
class CNLOpt {	
protected:
	TTop_Solution mInitial_Top_Solution;
	TBottom_Solution mInitial_Bottom_Solution;
protected:
	std::vector<size_t> mDimension_Remap;		//in the case that some bounds limit particular parameter to a constant, it decreases the problem dimension => we need to check as the results may differ then
												//for example with diff2, k=h=0 would produce worse solution if we do not reduce the number of parameters to non-zero ones
	std::vector<double> mRemapped_Upper_Top_Bound, mRemapped_Lower_Top_Bound;	//reduced bounds
protected:
	TFitness &mFitness;
	glucose::SMetric &mMetric;
	TTop_Solution mLower_Top_Bound, mUpper_Top_Bound;
	TBottom_Solution mLower_Bottom_Bound, mUpper_Bottom_Bound;

	std::vector<double> Top_Solution_As_Vector(const TTop_Solution& solution) {
		std::vector<double> result(solution.data(), solution.data() + solution.cols()*solution.rows()); 
		return result; 
	}
public:
	CNLOpt(const std::vector<TResult_Solution> &initial_solutions, const TResult_Solution &lower_bound, const TResult_Solution &upper_bound, TFitness &fitness, glucose::SMetric &metric) :
		mFitness(fitness), mMetric(metric) {
		
		TResult_Solution tmp_init;

		if (initial_solutions.size()>0) tmp_init = upper_bound.min(lower_bound.max(initial_solutions[0]));
			else  tmp_init = lower_bound + (upper_bound - lower_bound)*0.5;

		mInitial_Bottom_Solution = mInitial_Top_Solution.Decompose(tmp_init);
		mUpper_Bottom_Bound = mUpper_Top_Bound.Decompose(upper_bound);
		mLower_Bottom_Bound = mLower_Top_Bound.Decompose(lower_bound);

		for (auto i = 0; i < mUpper_Top_Bound.cols(); i++) {


			if (mUpper_Top_Bound[i] != mLower_Top_Bound[i]) {
				mDimension_Remap.push_back(i);
				mRemapped_Upper_Top_Bound.push_back(mUpper_Top_Bound[i]);
				mRemapped_Lower_Top_Bound.push_back(mRemapped_Lower_Top_Bound[i]);
			} 
				
		}
	}


	TResult_Solution Solve(volatile glucose::TSolver_Progress &progress) {

		if (nlopt_tx::print_statistics) nlopt_tx::eval_counter = 0;


		nlopt::opt opt{ mTop_Algorithm, static_cast<unsigned int>(mDimension_Remap.size()) };


		const std::vector<TBottom_Solution> initial_bottom_solution = { mInitial_Bottom_Solution };
		auto init_solution = mInitial_Top_Solution.Compose(mInitial_Bottom_Solution);
		auto metric_calculator = mMetric.Clone();					

		TNLOpt_Objective_Function_Data<TResult_Solution, TTop_Solution, TBottom_Solution, TFitness> data =
		{
			init_solution,											//	TResult_Solution best_solution;
			mFitness.Calculate_Fitness(init_solution, metric_calculator),						//floattype best_solution_fitness;

			mFitness,												//TFitness &fitness;
			mLower_Bottom_Bound, mUpper_Bottom_Bound,				//TBottom_Solution& lower_bottom_bound, upper_bottom_bound;
			initial_bottom_solution,								//std::vector<TBottom_Solution> initial_solution;
			mInitial_Top_Solution,
			mDimension_Remap,
			metric_calculator,										//SMetricCalculator metric_calculator; //don't make this a reference!	
			&progress,												//volatile TSolverProgress *progress;
			opt														//nlopt::opt &options;
		};
		
		//we need expression templates to reduce the overhead of remapping
		if (mDimension_Remap.size() == mUpper_Top_Bound.cols()) opt.set_min_objective(NLOpt_Top_Solution_Objective_Function<TResult_Solution, TTop_Solution, TBottom_Solution, TBottom_Solver, TFitness, false>, &data);
			else opt.set_min_objective(NLOpt_Top_Solution_Objective_Function<TResult_Solution, TTop_Solution, TBottom_Solution, TBottom_Solver, TFitness, true>, &data);
				
		

		opt.set_lower_bounds(mRemapped_Lower_Top_Bound);
		opt.set_upper_bounds(mRemapped_Upper_Top_Bound);
		

		//opt.set_xtol_rel(1e-4);
		opt.set_xtol_rel(1e-10);
		if (max_eval>0) opt.set_maxeval(max_eval);


		progress.max_progress = opt.get_maxeval();
		if (progress.max_progress == 0) progress.max_progress = 100;
		progress.current_progress = 0;
		progress.best_metric = std::numeric_limits<decltype(progress.best_metric)>::max();

		std::vector<double> x = Top_Solution_As_Vector(mInitial_Top_Solution);
		double minf = std::numeric_limits<double>::max();
		nlopt::result result = opt.optimize(x, minf); //note that x contains only the top-level parameters, but we need full, composed result
		
		if (nlopt_tx::print_statistics) {
			const size_t count = nlopt_tx::eval_counter;	//https://stackoverflow.com/questions/27314485/use-of-deleted-function-error-with-stdatomic-int
			dprintf((wchar_t*) L"%d; %g\n", (const size_t) count, (double) data.best_solution_fitness); //https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-2/compiler-error-c2665?f1url=https%3A%2F%2Fmsdn.microsoft.com%2Fquery%2Fdev15.query%3FappId%3DDev15IDEF1%26l%3DEN-US%26k%3Dk(C2665)%3Bk(vs.output)%26rd%3Dtrue
		}

		return data.best_solution;
	}
	
};