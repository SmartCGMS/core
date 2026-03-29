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

#include <scgms/iface/SolverIface.h>

#include "distributed.h"
#include "descriptor.h"

#include <functional>
#include <array>
#include <iostream>


//#############################################################################################
//# Generic wrapper - wraps construction of a solver and calls its "Solve" function
//#############################################################################################
template <typename TSolver>
bool Solve_Distributed(solver::TSolver_Setup& setup, solver::TSolver_Progress& progress)
{
    try
    {
        TSolver solver{setup};
        return solver.Solve(progress);
    }
    catch (std::exception& e)
    {
        std::cout << "Distributed Solver Error!\n" << e.what() << std::endl;
        return false;
    }
}


//#############################################################################################
//# Solver mapping - "solver table item"
//#############################################################################################
using TSolver_Func = std::function<bool(solver::TSolver_Setup&, solver::TSolver_Progress&)>;

struct TSolver_Info
{
    TSolver_Info() = delete;

    explicit TSolver_Info(const GUID& _id, const TSolver_Func& _fnc)
        : id(_id), func(_fnc)
    {
    }

    GUID id = Invalid_GUID;
    TSolver_Func func;
};


//#############################################################################################
//# Solver table - contains pairs of {solver_description, solver's Solve_Distributed call}
//#############################################################################################
const std::array<TSolver_Info, 1> solvers = {
    TSolver_Info{
        scgms_distributed_solver::distributed_solver_generic,
        Solve_Distributed<scgms_distributed_solver::CDistributed_Solver> // TODO???
    }
};


//#############################################################################################
//# SCGMS entry point - call different Solve_Distributed functions (from "solvers" table) depending on GUID
//#############################################################################################
DLL_EXPORT HRESULT IfaceCalling do_solve_generic(
    const GUID* solver_id,
    solver::TSolver_Setup* setup,
    solver::TSolver_Progress* progress)
{
    for (const auto& solver : solvers)
    {
        if (solver.id == *solver_id)
        {
            try
            {
                return solver.func(*setup, *progress) ? S_OK : E_INVALIDARG;
            }
            catch (...)
            {
                return E_FAIL;
            }
        }
    }

    return E_NOTIMPL;
}
