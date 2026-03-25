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

#include <scgms/rtl/SolverLib.h>

#include <algorithm>
#include <vector>
#include <numeric>
#include <limits>
#include <cassert>

#include <pagmo/types.hpp>
#include <pagmo/problem.hpp>

#include "descriptor.h"
#include "distributed_solver.h"
#include "pagmo/algorithms/nsga2.hpp"

namespace scgms_distributed_solver
{
    //#############################################################################################
    //# CRemap - "dimension reduction / dimension expansion"
    //#############################################################################################
    /**
     * - This class is needed because:
     *  - SCGMS problem (defined in TSolver_Setup) can have parameters which are "fixed"
     *      - Those parameters cannot change because their lower_bound == upper_bound
     *      - Therefore, it makes no sense to use them with a Pagmo optimization algorithm
     *
     *  - Expand_Solution(x)
     *      - This function converts the "pagmo" representation of individual into a "full" representation
     *      - "full" representation contains both the "fixed" parameters and the parameters we're optimizing
     *
     *  - Reduce_Solution(solution)
     *      - Takes a "full" individual vector and extracts only those parameters which aren't "fixed"
     *
     *  - get_bounds() and problem_size()
     *      - Adapters for pagmo, bounds and problem size only for the non-fixed parameters
     */
    class CRemap
    {
    protected:
        pagmo::vector_double mRemapped_Lower, mRemapped_Upper;
        std::vector<size_t> mDimension_Remap;
        const solver::TSolver_Setup mSetup;

    public:
        CRemap(const solver::TSolver_Setup& setup) : mSetup(setup)
        {
            for (size_t i = 0; i < mSetup.problem_size; i++)
            {
                if (mSetup.lower_bound[i] != mSetup.upper_bound[i])
                {
                    mDimension_Remap.push_back(i);
                    mRemapped_Lower.push_back(mSetup.lower_bound[i]);
                    mRemapped_Upper.push_back(mSetup.upper_bound[i]);
                }
            }
        }

        pagmo::vector_double Expand_Solution(const pagmo::vector_double& x) const
        {
            pagmo::vector_double solution(mSetup.lower_bound, mSetup.lower_bound + mSetup.problem_size);

            for (size_t i = 0; i < x.size(); i++)
            {
                solution[mDimension_Remap[i]] = std::min(mRemapped_Upper[i], std::max(x[i], mRemapped_Lower[i]));
            }
            return solution;
        }

        pagmo::vector_double Reduce_Solution(const double* solution) const
        {
            pagmo::vector_double result;

            for (size_t i = 0; i < mDimension_Remap.size(); i++)
                result.push_back(solution[mDimension_Remap[i]]);

            return result;
        }

        std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const
        {
            return {mRemapped_Lower, mRemapped_Upper};
        }

        size_t problem_size() const
        {
            return mDimension_Remap.size();
        }
    };


    //#############################################################################################
    //# TProblem - Pagmo-style UDP
    //#############################################################################################
    class TProblem
    {
    protected:
        const solver::TSolver_Setup& mSetup;
        // We're using this remapper (defined above) to convert SCGMS individual vector representation into a pagmo-compatible one
        const CRemap mRemap;

    protected:
        solver::TSolver_Progress mVoid_Progress = solver::Null_Solver_Progress;
        solver::TSolver_Progress& mProgress;

    public:
        TProblem(const solver::TSolver_Setup& setup, solver::TSolver_Progress& progress)
            : mSetup(setup),
              mRemap(setup),
              mProgress(progress)
        {
        }

        TProblem()
            : mSetup(solver::Default_Solver_Setup),
              mRemap(solver::Default_Solver_Setup),
              mProgress(mVoid_Progress)
        {
        }

        TProblem(const TProblem& other)
            : mSetup(other.mSetup),
              mRemap(other.mRemap),
              mProgress(other.mProgress)
        {
        }

        /**
         * Pagmo UDP's standard fitness function,
         * called internally by Pagmo algorithms
         */
        pagmo::vector_double fitness(const pagmo::vector_double& x) const
        {
            assert(mSetup.problem_size > 0);

            const auto solution = mRemap.Expand_Solution(x);

            pagmo::vector_double result(
                solver::Maximum_Objectives_Count,
                std::numeric_limits<double>::quiet_NaN()
            );

            mSetup.objective(mSetup.data, 1, solution.data(), result.data());

            result.resize(mSetup.objectives_count);
            return result;
        }

        //TODO: Batch fitness could be added here

        /**
         * Pagmo UDP's standard bounds function,
         * called internally by Pagmo algorithms
         */
        std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const
        {
            return mRemap.get_bounds();
        }

        /**
         * Pagmo UDP's standard number of objectives function,
         * called internally by Pagmo algorithms
         */
        pagmo::vector_double::size_type get_nobj() const
        {
            return mSetup.objectives_count;
        }

        size_t problem_size() const
        {
            return mRemap.problem_size();
        }

        const CRemap& remap() const
        {
            return mRemap;
        }
    };


    //#############################################################################################
    //# CDistributed_Solver - SCGMS adapter
    //#############################################################################################
    class CDistributed_Solver
    {
    protected:
        solver::TSolver_Setup mSetup;
        CRemap mRemap;

    public:
        CDistributed_Solver(const solver::TSolver_Setup& setup)
            : mSetup(solver::Check_Default_Parameters(setup, 100'000, 100)),
              mRemap(setup)
        {
        }

        bool Solve(solver::TSolver_Progress& progress)
        {
            // TODO: Maybe wrap in try catch - setting succeeded? Or no - factory.cpp has try catch block

            // 1) Initialize key variables
            //####################################################
            progress = solver::Null_Solver_Progress;
            const size_t popSize = mSetup.population_size;
            const size_t generationCount = mSetup.max_generations;
            bool succeeded = false;

            // 2) Construct a pagmo UDP using our TProblem wrapper
            //####################################################
            TProblem udp{mSetup, progress}; // TODO: DLL problem?
            const pagmo::problem prob{udp};

            // 3) Construct a pagmo Algorithm
            //####################################################
            pagmo::algorithm algo{pagmo::nsga2(generationCount)};
            algo.set_verbosity(0u);

            // 4) Set up distributed solver + hints
            //####################################################
            distributed_solver distSolver{"tcp://localhost:5000", 1}; // TODO: Params

            std::vector<pagmo::vector_double> hints{};
            if (mSetup.hint_count > 0)
            {
                for (size_t i = 0; i < mSetup.hint_count; i++)
                {
                    hints.emplace_back(mRemap.Reduce_Solution(mSetup.hints[i]));
                }
            }
            distSolver.set_initial_hints(hints);

            // 5) Run the distributed evolution
            //####################################################
            distSolver.evolve(prob, {algo}, popSize); // TODO: Maybe set cycleCount?
            // Blocking call
            const auto& bestIndividual = distSolver.wait_until_completion();

            // 6) Set if evolution was successful
            //####################################################
            // TODO: Does this have any effect? wait_check() resets evolve_status and maybe throws if error?
            succeeded = distSolver.get_status() == pagmo::evolve_status::idle;

            // 7) Write back result and return
            //####################################################
            pagmo::vector_double champion_x(mRemap.problem_size(),std::numeric_limits<double>::quiet_NaN());
            champion_x = mRemap.Expand_Solution(bestIndividual);

            if (succeeded)
                std::copy(champion_x.begin(), champion_x.end(), mSetup.solution);

            return succeeded;
        }
    };
} // namespace scgms_distributed_solver


//#############################################################################################
//# SCGMS entry point
//#############################################################################################

DLL_EXPORT HRESULT IfaceCalling do_solve(
    const GUID* solver_id,
    const solver::TSolver_Setup* setup,
    solver::TSolver_Progress* progress)
{
    if (!setup || !progress)
        return E_INVALIDARG;

    if (*solver_id == scgms_distributed_solver::distributed_solver_generic)
    {
        scgms_distributed_solver::CDistributed_Solver solver{*setup}; // TODO?
        return solver.Solve(*progress) ? S_OK : E_FAIL;
    }

    return E_NOTIMPL;
}
