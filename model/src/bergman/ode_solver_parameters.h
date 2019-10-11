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
#define DEFINE_ODE_SOLVER_ADAPTIVE(name) template <class Adaptive_Strategy = DEFAULT_ADAPTIVE_STRATEGY<order>> class name : public CRunge_Kutta_ODE_Solver<order, true, Adaptive_Strategy> { public: name(double errorThreshold) : CRunge_Kutta_ODE_Solver<order, true, Adaptive_Strategy>(rkmatrix, weights, weights_alt, nodes, errorThreshold) {} };

	// Euler's method (1st order)
	namespace euler
	{
		constexpr size_t order = 1;

		const std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0 }
		} };

		const std::array<double, order> weights = { 1 };
		const std::array<double, order> nodes = { 0 };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// Heun's method (2nd order)
	namespace heun
	{
		constexpr size_t order = 2;

		std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0,         0 },
			{ 1. / 2.,   0 }
		} };

		const std::array<double, order> weights = { 0, 1 };
		const std::array<double, order> nodes = { 0, 1. / 2. };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// Kutta's method ("original") (3rd order)
	namespace kutta
	{
		constexpr size_t order = 3;

		std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0,       0, 0 },
			{ 1. / 2., 0, 0 },
			{ -1,      2, 0 }
		} };

		const std::array<double, order> weights = { 1. / 6., 2. / 3., 1. / 6. };
		const std::array<double, order> nodes = { 0, 1. / 2., 1 };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// 3/8 rule (4th order)
	namespace rule38
	{
		constexpr size_t order = 4;

		std::array<std::array<double, order>, order> rkmatrix = { {
			{ 0,         0, 0, 0 },
			{ 1. / 3.,   0, 0, 0 },
			{ -1. / 3.,  1, 0, 0 },
			{ 1,        -1, 1, 0 }
		} };

		const std::array<double, order> weights = { 1. / 8., 3. / 8., 3. / 8., 1. / 8. };
		const std::array<double, order> nodes = { 0, 1. / 3., 2. / 3., 1 };

		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver);
	}

	// Dormand-Prince method (5th order with 4th order error estimation)
	namespace dormandprince
	{
		constexpr size_t order = 7;

		std::array<std::array<double, order>, order> rkmatrix = { {
			{{ 0, 0, 0, 0, 0, 0, 0 }},
			{{ 1. / 5., 0, 0, 0, 0, 0, 0 }},
			{{ 3. / 40., 9. / 40., 0, 0, 0, 0, 0 }},
			{{ 44. / 45., -56. / 15., 32. / 9., 0, 0, 0, 0 }},
			{{ 19372. / 6561., -25360. / 2187., 64448. / 6561., -212. / 729., 0, 0, 0 }},
			{{ 9017. / 3168., -355. / 33., 46732. / 5247., 49. / 176., -5103. / 18656., 0, 0 }},
			{{ 35. / 384., 0, 500. / 1113., 125. / 192., -2187. / 6784., 11. / 84., 0 }}
		} };

		const std::array<double, order> weights = { { 35. / 384., 0, 500. / 1113., 125. / 192., -2187. / 6784., 11. / 84., 0 } };
		const std::array<double, order> weights_alt = { { 5179. / 57600., 0., 7571. / 16695., 393. / 640., -92097. / 339200., 187. / 2100., 1. / 40. } };
		const std::array<double, order> nodes = { { 0, 1. / 5., 3. / 10., 4. / 5., 8. / 9., 1, 1 } };

		DEFINE_ODE_SOLVER_ADAPTIVE(CSolver);

		// although it is unlikely, one may want to use Dormand-Prince method without error estimation and step adjustment
		DEFINE_ODE_SOLVER_NONADAPTIVE(CSolver_Non_Adaptive);
	}
}
