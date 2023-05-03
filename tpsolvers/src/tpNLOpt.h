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

#pragma once

#include <nlopt.hpp>

#include "../../../common/iface/SolverIface.h"
#include "../../../common/utils/DebugHelper.h"

#include <atomic>

#undef min

namespace nlopt_tx {
	constexpr bool print_statistics = false;
	extern std::atomic<size_t> eval_counter;
}

struct TNLOpt_Objective_Function_Data {
	const solver::TSolver_Setup& setup;
	solver::TSolver_Progress& progress;
	std::vector<double> remapped_solution;	//must be initialized to the default solution as only the remapped elements will be replaced	
	const std::vector<size_t>& dimension_remap;
	nlopt::opt& options;
	double& best_distance;
};


template <typename T>
inline double Solution_Distance(const size_t objective_count, const T solution) {
	if (objective_count == 1)
		return solution[0];

	double result = 0.0;
	for (size_t i = 0; i < objective_count; i++) {
		result += solution[i] * solution[i];
	}

	return result;
}


//let's use use_remap to eliminate the overhead of remapping, when it is not needed - compiler will generate two functions for us, just from a single source code
template <bool use_remap>
double NLOpt_Top_Solution_Objective_Function(const std::vector<double>& x, std::vector<double>& grad, void* my_func_data) {
	TNLOpt_Objective_Function_Data& data = *static_cast<TNLOpt_Objective_Function_Data*>(my_func_data);

	if (use_remap)
		for (size_t i = 0; i < x.size(); i++) {
			data.remapped_solution[data.dimension_remap[i]] = x[i];
		}


	std::array<double, solver::Maximum_Objectives_Count> fitness = solver::Nan_Fitness;
	if (data.setup.objective(data.setup.data, 1, use_remap ? data.remapped_solution.data() : x.data(), fitness.data()) != TRUE)
		return std::numeric_limits<double>::quiet_NaN();

	const double distance = Solution_Distance(data.setup.objectives_count, fitness);
	if (distance < data.best_distance) {
		data.best_distance = distance;
		data.progress.best_metric = fitness;
	}
	

	data.progress.current_progress++;

	if (nlopt_tx::print_statistics)	nlopt_tx::eval_counter++;

	if (data.progress.cancelled == TRUE) data.options.set_force_stop(true);

	return distance;
}


template <nlopt::algorithm algo>
class CNLOpt {
protected:
	std::vector<double> mInitial_Solution;	
protected:
	std::vector<size_t> mDimension_Remap;		//in the case that some bounds limit particular parameter to a constant, it decreases the problem dimension => we need to check as the results may differ then
												//for example with diff2, k=h=0 would produce worse solution if we do not reduce the number of parameters to non-zero ones
	std::vector<double> mRemapped_Upper_Top_Bound, mRemapped_Lower_Top_Bound;	//reduced bounds
protected:
	const solver::TSolver_Setup &mSetup;
public:
	CNLOpt(const solver::TSolver_Setup &setup) : mSetup(setup) {
		
		mInitial_Solution.resize(mSetup.problem_size);

		if (mSetup.hint_count > 0) {
			for (size_t i = 0; i < mInitial_Solution.size(); i++)		//enforce bounds as the initial hint can be wrong
				mInitial_Solution[i] = std::min(mSetup.upper_bound[i], std::max(mSetup.hints[0][i], mSetup.lower_bound[i]));
		}
		else {
			for (size_t i = 0; i < mInitial_Solution.size(); i++)		//enforce bounds as pagmo does not do so - pagmo prevents us from doing so permanently in the objective function/object of the TProblem class
				mInitial_Solution[i] = mSetup.lower_bound[i] + 0.5*(mSetup.upper_bound[i] - mSetup.lower_bound[i]);
		}

		for (auto i = 0; i < mSetup.problem_size; i++) {

			if (mSetup.lower_bound[i] != mSetup.upper_bound[i]) {
				mDimension_Remap.push_back(i);
				mRemapped_Upper_Top_Bound.push_back(mSetup.upper_bound[i]);
				mRemapped_Lower_Top_Bound.push_back(mSetup.lower_bound[i]);
			}
		}
	}


	bool Solve(solver::TSolver_Progress &progress) {
		progress = solver::Null_Solver_Progress;

		if ((algo == nlopt::LN_PRAXIS) && (mDimension_Remap.size() < 2)) return false;	//NLopt lib does not actully check the required N

		if (nlopt_tx::print_statistics) nlopt_tx::eval_counter = 0;

		nlopt::opt opt{ algo, static_cast<unsigned int>(mDimension_Remap.size()) };
		
		double best_distance = std::numeric_limits<double>::max();

		TNLOpt_Objective_Function_Data data = {	
			mSetup,
			progress,
			mInitial_Solution,
			mDimension_Remap,
			opt,
			best_distance
		};
		
		//we need expression templates to reduce the overhead of remapping
		if (mDimension_Remap.size() == mSetup.problem_size) opt.set_min_objective(NLOpt_Top_Solution_Objective_Function<false>, &data);
			else opt.set_min_objective(NLOpt_Top_Solution_Objective_Function<true>, &data);
		
		opt.set_lower_bounds(mRemapped_Lower_Top_Bound);
		opt.set_upper_bounds(mRemapped_Upper_Top_Bound);
		
		opt.set_ftol_abs(mSetup.tolerance);		
		if (mSetup.max_generations != 0) opt.set_maxeval(static_cast<int>(mSetup.max_generations)); 
		
		progress.max_progress = opt.get_maxeval();
		if (progress.max_progress == 0) progress.max_progress = 100;
		progress.current_progress = 0;		

		std::vector<double> x;
		for (auto i = 0; i < mDimension_Remap.size(); i++)
			x.push_back(mInitial_Solution[mDimension_Remap[i]]);

		double minf = std::numeric_limits<double>::max();
		nlopt::result result;
		try {
			result = opt.optimize(x, minf);
		}
		catch (nlopt::roundoff_limited) {
			//just mask it as accordingly to the manual the result is useable
			result = nlopt::result::FTOL_REACHED;
		}
		
		if (nlopt_tx::print_statistics) {
			const size_t count = nlopt_tx::eval_counter;	//https://stackoverflow.com/questions/27314485/use-of-deleted-function-error-with-stdatomic-int
			dprintf((wchar_t*) L"%d; %g\n", (const size_t) count, (double) minf); //https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-2/compiler-error-c2665?f1url=https%3A%2F%2Fmsdn.microsoft.com%2Fquery%2Fdev15.query%3FappId%3DDev15IDEF1%26l%3DEN-US%26k%3Dk(C2665)%3Bk(vs.output)%26rd%3Dtrue
		}


		if (mDimension_Remap.size() == mSetup.problem_size) {
			//no remap
			memcpy(mSetup.solution, x.data(), mSetup.problem_size * sizeof(double));
		} else {
			memcpy(mSetup.solution, mSetup.lower_bound, mSetup.problem_size * sizeof(double));	//copy the parameters, which possibly stays the same
			for (auto i = 0; i < mDimension_Remap.size(); i++)					//and copy those, which changed at their correct positions in the final solution
				mSetup.solution[mDimension_Remap[i]] = x[i];
		}

		return (result == nlopt::result::SUCCESS || result == nlopt::result::FTOL_REACHED || result == nlopt::result::MAXEVAL_REACHED);
	}
};