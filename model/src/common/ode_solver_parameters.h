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
 * Univerzitni 8
 * 301 00, Pilsen
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


#pragma once

#include <array>

#include "ode_solvers.h"

namespace ode
{
	/*
	 * rkmatrix = matrix a_ij from Butcher's tableau
	 * weights = vector b_i from Butcher's tableau
	 * weights_alt = vector b*_i from Butcher's tableau (higher order error estimation)
	 * nodes = vector c_i from Butcher's tableau
	 */

	// default adaptive step strategy for adaptive RK methods
#define DEFAULT_ADAPTIVE_STRATEGY CRunge_Kuttta_Adaptive_Strategy_Binary_Subdivision

	// defines solver class for non-adaptive RK method
#define DEFINE_ODE_SOLVER_NONADAPTIVE(name) class name : public CRunge_Kutta_ODE_Solver<order> { public: name() : CRunge_Kutta_ODE_Solver<order>(rkmatrix, weights, nodes) {} };
	// defines solver class for adaptive RK method
#define DEFINE_ODE_SOLVER_ADAPTIVE(name) template <class Adaptive_Strategy = DEFAULT_ADAPTIVE_STRATEGY<order>> class name : public CRunge_Kutta_ODE_Solver<order, true, Adaptive_Strategy> { public: name(const double errorThreshold, const size_t max_steps) : CRunge_Kutta_ODE_Solver<order, true, Adaptive_Strategy>(rkmatrix, weights, weights_alt, nodes, errorThreshold, max_steps) {} };

	// Euler's method (1st order)
	namespace euler
	{
		constexpr size_t order = 1;

		static const std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0 }
		} };

		static const std::array<double, order> weights = { 1 };
		static const std::array<double, order> nodes = { 0 };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// Heun's method (2nd order)
	namespace heun
	{
		constexpr size_t order = 2;

		static std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0,         0 },
			{ 1. / 2.,   0 }
		} };

		static const std::array<double, order> weights = { 0, 1 };
		static const std::array<double, order> nodes = { 0, 1. / 2. };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// Kutta's method ("original") (3rd order)
	namespace kutta
	{
		constexpr size_t order = 3;

		static std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0,       0, 0 },
			{ 1. / 2., 0, 0 },
			{ -1,      2, 0 }
		} };

		static const std::array<double, order> weights = { 1. / 6., 2. / 3., 1. / 6. };
		static const std::array<double, order> nodes = { 0, 1. / 2., 1 };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// 3/8 rule (4th order)
	namespace rule38
	{
		constexpr size_t order = 4;

		static std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0,         0, 0, 0 },
			{ 1. / 3.,   0, 0, 0 },
			{ -1. / 3.,  1, 0, 0 },
			{ 1,        -1, 1, 0 }
		} };

		static const std::array<double, order> weights = { 1. / 8., 3. / 8., 3. / 8., 1. / 8. };
		static const std::array<double, order> nodes = { 0, 1. / 3., 2. / 3., 1 };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// Dormand-Prince method (5th order with 4th order error estimation)
	namespace dormandprince
	{
		constexpr size_t order = 7;

		static std::array<std::array<double, order>, order> rkmatrix = { {
			{{ 0, 0, 0, 0, 0, 0, 0 }},
			{{ 1. / 5., 0, 0, 0, 0, 0, 0 }},
			{{ 3. / 40., 9. / 40., 0, 0, 0, 0, 0 }},
			{{ 44. / 45., -56. / 15., 32. / 9., 0, 0, 0, 0 }},
			{{ 19372. / 6561., -25360. / 2187., 64448. / 6561., -212. / 729., 0, 0, 0 }},
			{{ 9017. / 3168., -355. / 33., 46732. / 5247., 49. / 176., -5103. / 18656., 0, 0 }},
			{{ 35. / 384., 0, 500. / 1113., 125. / 192., -2187. / 6784., 11. / 84., 0 }}
		} };

		static const std::array<double, order> weights = { { 35. / 384., 0, 500. / 1113., 125. / 192., -2187. / 6784., 11. / 84., 0 } };
		static const std::array<double, order> weights_alt = { { 5179. / 57600., 0., 7571. / 16695., 393. / 640., -92097. / 339200., 187. / 2100., 1. / 40. } };
		static const std::array<double, order> nodes = { { 0, 1. / 5., 3. / 10., 4. / 5., 8. / 9., 1, 1 } };

		DEFINE_ODE_SOLVER_ADAPTIVE(CSolver);

		// although it is unlikely, one may want to use Dormand-Prince method without error estimation and step adjustment
		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver_Non_Adaptive);
	}

	// different ODE solvers we might want to use; we prefer Dormand-Prince parametrization with binary subdivision adaptive step strategy (best balance of speed and precision)
	//using default_solver = euler::CSolver ODE_Solver;
	//using default_solver = heun::CSolver ODE_Solver;
	//using default_solver = kutta::CSolver ODE_Solver;
	//using default_solver = rule38::CSolver ODE_Solver;
	//using default_solver = dormandprince::CSolver_Non_Adaptive ODE_Solver;
	using default_solver = dormandprince::CSolver<CRunge_Kuttta_Adaptive_Strategy_Binary_Subdivision<4>>;
	//using default_solver = dormandprince::CSolver<CRunge_Kuttta_Adaptive_Strategy_Optimal_Estimation<4>> ODE_Solver{ ODE_epsilon0 };
}
