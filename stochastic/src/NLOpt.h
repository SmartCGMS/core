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


template <typename TResult_Solution, typename TBottom_Solution, typename TFitness>
struct TNLOpt_Objective_Function_Data {

	TResult_Solution best_solution;
	floattype best_solution_fitness;

	TFitness &fitness;
	TBottom_Solution& lower_bottom_bound, upper_bottom_bound;
	std::vector<TBottom_Solution> initial_bottom_solution;
	SMetricCalculator metric_calculator; //don't make this a reference!	
	SMetricFactory &metric_factory;

	volatile TSolverProgress *progress;
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

	floattype Calculate_Fitness(const TBottom_Solution &bottom_solution, SMetricCalculator &metric) const {

		return mOriginal_Fitness.Calculate_Fitness(mTop_Solution.Compose(bottom_solution), metric);
	};
};

template <typename TResult_Solution, typename TTop_Solution, typename TBottom_Solution, typename TBottom_Solver, typename TFitness>
double NLOpt_Top_Solution_Objective_Function(const std::vector<double> &x, std::vector<double> &grad, void *my_func_data) {
	TNLOpt_Objective_Function_Data<TResult_Solution, TBottom_Solution, TFitness> * data = (TNLOpt_Objective_Function_Data<TResult_Solution, TBottom_Solution, TFitness>*)my_func_data;

	TTop_Solution top_result = TTop_Solution::From_Vector(x);

	CNLOpt_Fitness_Proxy<TFitness, TTop_Solution, TBottom_Solution> fitness_proxy{ data->fitness, top_result };
	TBottom_Solver bottom_solver(data->initial_bottom_solution, data->lower_bottom_bound, data->upper_bottom_bound, fitness_proxy, data->metric_factory);
	TSolverProgress tmp_progress = { 0 };
	TBottom_Solution bottom_result = bottom_solver.Solve(tmp_progress);

	TResult_Solution composed_result = top_result.Compose(bottom_result);
	

	double fitness = data->fitness.Calculate_Fitness(composed_result, data->metric_calculator);


	if (fitness < data->best_solution_fitness) {
		data->best_solution_fitness = fitness;
		data->best_solution = composed_result;

		data->progress->BestMetric = fitness;
	}

	data->progress->CurrentProgress++;	

	if (nlopt_tx::print_statistics)	nlopt_tx::eval_counter++;

	if (data->progress->Cancelled) data->options.set_force_stop(true);

	return fitness;
}


template <typename TResult_Solution, typename TFitness, nlopt::algorithm mTop_Algorithm, typename TTop_Solution = TResult_Solution, typename TBottom_Solution = TResult_Solution, typename TBottom_Solver = CNullMethod<TResult_Solution, CNLOpt_Fitness_Proxy<TFitness, TResult_Solution, TResult_Solution>>, size_t max_eval = 0>
class CNLOpt {	
protected:
	TTop_Solution mInitial_Top_Solution;
	TBottom_Solution mInitial_Bottom_Solution;
protected:
	TFitness &mFitness;
	SMetricFactory &mMetric_Factory;
	TTop_Solution mLower_Top_Bound, mUpper_Top_Bound;
	TBottom_Solution mLower_Bottom_Bound, mUpper_Bottom_Bound;
public:
	CNLOpt(const std::vector<TResult_Solution> &initial_solutions, const TResult_Solution &lower_bound, const TResult_Solution &upper_bound, TFitness &fitness, SMetricFactory &metric_factory) :
		mFitness(fitness), mMetric_Factory(metric_factory) {
		
		TResult_Solution tmp_init;

		if (initial_solutions.size()>0) tmp_init = upper_bound.min(lower_bound.max(initial_solutions[0]));
			else  tmp_init = lower_bound + (upper_bound - lower_bound)*0.5;

		mInitial_Bottom_Solution = mInitial_Top_Solution.Decompose(tmp_init);
		mUpper_Bottom_Bound = mUpper_Top_Bound.Decompose(upper_bound);
		mLower_Bottom_Bound = mLower_Top_Bound.Decompose(lower_bound);
	}


	TResult_Solution Solve(volatile TSolverProgress &progress) {

		if (nlopt_tx::print_statistics) nlopt_tx::eval_counter = 0;


		nlopt::opt opt(mTop_Algorithm, static_cast<unsigned int>(mInitial_Top_Solution.cols()));


		
		progress.MaxProgress = opt.get_maxeval();
		if (progress.MaxProgress == 0) progress.MaxProgress = 100;
		progress.CurrentProgress = 0;
		progress.BestMetric = std::numeric_limits<decltype(progress.BestMetric)>::max();

		const std::vector<TBottom_Solution> initial_bottom_solution = { mInitial_Bottom_Solution };
		const auto init_solution = mInitial_Top_Solution.Compose(mInitial_Bottom_Solution);
		auto metric_calculator = mMetric_Factory.CreateCalculator();

		TNLOpt_Objective_Function_Data<TResult_Solution, TBottom_Solution, TFitness> data =
		{
			init_solution,											//	TResult_Solution best_solution;
			mFitness.Calculate_Fitness(init_solution, metric_calculator),						//floattype best_solution_fitness;

			mFitness,												//TFitness &fitness;
			mLower_Bottom_Bound, mUpper_Bottom_Bound,				//TBottom_Solution& lower_bottom_bound, upper_bottom_bound;
			initial_bottom_solution,								//std::vector<TBottom_Solution> initial_solution;
			metric_calculator,										//SMetricCalculator metric_calculator; //don't make this a reference!	
			mMetric_Factory,										//SMetricFactory &metric_factory
			&progress,												//volatile TSolverProgress *progress;
			opt														//nlopt::opt &options;
		};
		
				
		opt.set_min_objective(NLOpt_Top_Solution_Objective_Function<TResult_Solution, TTop_Solution, TBottom_Solution, TBottom_Solver, TFitness>, &data);

		opt.set_lower_bounds(mLower_Top_Bound.As_Vector());
		opt.set_upper_bounds(mUpper_Top_Bound.As_Vector());
		

		//opt.set_xtol_rel(1e-4);
		opt.set_xtol_rel(1e-10);
		if (max_eval>0) opt.set_maxeval(max_eval);

		std::vector<double> x = mInitial_Top_Solution.As_Vector();
		double minf = std::numeric_limits<double>::max();
		nlopt::result result = opt.optimize(x, minf); //note that x contains only the top-level parameters, but we need full, composed result
		
		if (nlopt_tx::print_statistics) {
			const size_t count = nlopt_tx::eval_counter;	//https://stackoverflow.com/questions/27314485/use-of-deleted-function-error-with-stdatomic-int
			dprintf("%d; %g\n", count, data.best_solution_fitness);
		}

		return data.best_solution;
	}
	
};