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
#include <iomanip>
#include <limits>

/*
 * Base for all RK adaptive step methods
 * All derived classes must define strategy of step reduction and evaluation of next step count
 */
template <size_t N>
class CRunge_Kutta_Adaptive_Strategy
{
	public:
		// adjusts the step size; returns true if successfull, false if no step size reduction is possible (or viable)
		virtual bool Adjust_Step(const double currentError, const double errorThreshold, const double remainingStep, double& step, size_t& remStepCnt) const = 0;
};

/*
 * Adaptive strategy: binary subdivision
 * Based on: https://www.uni-muenster.de/imperia/md/content/physik_tp/lectures/ss2017/numerische_Methoden_fuer_komplexe_Systeme_II/rkm-1.pdf
 *
 * When the error is above threshold, set step size to half and double the step count
 * Otherwise try to double the step size and halve step count, if the remaining step count is even
 */
template <size_t N>
class CRunge_Kuttta_Adaptive_Strategy_Binary_Subdivision : public CRunge_Kutta_Adaptive_Strategy<N>
{
	public:
		virtual bool Adjust_Step(const double currentError, const double errorThreshold, const double remainingStep, double& step, size_t& remStepCnt) const override final
		{
			if (currentError > errorThreshold)
			{
				step /= 2.0;
				remStepCnt *= 2;
			}
			else
			{
				if ((remStepCnt % 2) == 0)
				{
					step *= 2.0;
					remStepCnt /= 2;
				}
			}

			return true;
		}
};

/*
 * Adaptive strategy: optimal step size (Runge-Kutta-Fehlberg)
 * Based on: https://www.uni-muenster.de/imperia/md/content/physik_tp/lectures/ss2017/numerische_Methoden_fuer_komplexe_Systeme_II/rkm-1.pdf
 *
 * The step size is adjusted using current to threshold error ratio and method order
 */
template <size_t N>
class CRunge_Kuttta_Adaptive_Strategy_Optimal_Estimation : public CRunge_Kutta_Adaptive_Strategy<N>
{
	private:
		const double Beta = 0.9;

	public:
		virtual bool Adjust_Step(const double currentError, const double errorThreshold, const double remainingStep, double& step, size_t& remStepCnt) const override final
		{
			if (currentError > std::numeric_limits<double>::epsilon())
				step = Beta * step * pow(errorThreshold / currentError, 1.0 / ((currentError >= errorThreshold) ? N + 1 : N));

			if (step >= remainingStep) // we would exceed the total step
			{
				step = remainingStep;

				// when the result is about to get rejected, set step count to 1, so the next one is re-adjusted by current calculations with new step size
				if (currentError > errorThreshold)
					remStepCnt = 1;

				return true;
			}

			if (step < std::numeric_limits<double>::epsilon()) // step is too low
				step = std::numeric_limits<double>::epsilon();

			remStepCnt++; // this will guarantee another call to this method, so we can re-evaluate the step size

			return true;
		}
};

/*
 * Base for all RK solvers (adaptive and non-adaptive)
 */
template<size_t N>
class CRunge_Kutta_ODE_Solver_Base
{
	protected:
		using TCoef_Array = std::array<double, N>;
		using TCoef_Matrix = std::array<TCoef_Array, N>;
		using TK_Coef_Array = std::array<double, N + 1>;

	protected:
		// Runge-Kutta matrix (a_ij from Butcher's tableau)
		const TCoef_Matrix mRKMatrix;
		// weights vector (b_i from Buther's tableau)
		const TCoef_Array mWeights;
		// nodes vector (c_i from Butcher's tableau)
		const TCoef_Array mNodes;

	protected:
		// Evaluates the K function and stores the outputs into kfunc array
		template<typename _ObjFunc>
		void Evaluate_K_Func(_ObjFunc& objectiveFnc, TK_Coef_Array& kfunc, const double T, const double X, const double stepSize) const
		{
			kfunc[0] = 0;

			for (size_t j = 1; j <= N; j++)
			{
				double xacc = X;
				for (size_t ki = 0; ki < j; ki++)
					xacc += stepSize * (mRKMatrix[j - 1][ki] * kfunc[ki]);

				kfunc[j] = objectiveFnc(T + mNodes[j - 1] * stepSize, xacc);
			}
		}

	public:
		CRunge_Kutta_ODE_Solver_Base(const TCoef_Matrix& rkMatrix, const TCoef_Array& weights, const TCoef_Array& nodes)
			: mRKMatrix(rkMatrix), mWeights(weights), mNodes(nodes)
		{
		}

		// Perform one step of RK ODE solver - returns new value of input variable
		//virtual double Step(std::function<double(double, double)>& objectiveFnc, const double T, const double X, const double stepSize) = 0;
};

/*
 * Runge-Kutta non-adaptive solver class
 */
template<size_t N>
class CRunge_Kutta_ODE_Solver_NonAdaptive : public CRunge_Kutta_ODE_Solver_Base<N>
{
	protected:
		using TCoef_Array = typename CRunge_Kutta_ODE_Solver_Base<N>::TCoef_Array;
		using TCoef_Matrix = typename CRunge_Kutta_ODE_Solver_Base<N>::TCoef_Matrix;
		using TK_Coef_Array = typename CRunge_Kutta_ODE_Solver_Base<N>::TK_Coef_Array;

	protected:
		// Evaluates K coefficients and performs K-summation to estimate process variable difference
		double Evaluate_K_Coefs(const TK_Coef_Array& kfunc, const double stepSize) const
		{
			double sumK = 0;
			for (size_t j = 0; j < N; j++)
				sumK += CRunge_Kutta_ODE_Solver_Base<N>::mWeights[j] * kfunc[j + 1];

			return sumK * stepSize;
		}

	public:
		explicit CRunge_Kutta_ODE_Solver_NonAdaptive(const TCoef_Matrix& rkMatrix, const TCoef_Array& weights, const TCoef_Array& nodes)
			: CRunge_Kutta_ODE_Solver_Base<N>(rkMatrix, weights, nodes)
		{
		}

		template<typename _ObjFunc>
		double Step(_ObjFunc& objectiveFnc, const double T, const double X, const double stepSize) const
		{
			TK_Coef_Array kfunc;
			CRunge_Kutta_ODE_Solver_Base<N>::Evaluate_K_Func(objectiveFnc, kfunc, T, X, stepSize);

			return X + Evaluate_K_Coefs(kfunc, stepSize);
		}
};


/*
 * Runge-Kutta adaptive solver class
 */
template<size_t N,
	class Adaptive_Strategy = CRunge_Kuttta_Adaptive_Strategy_Binary_Subdivision<N>>
class CRunge_Kutta_ODE_Solver_Adaptive : public CRunge_Kutta_ODE_Solver_Base<N>
{
	protected:
		using TCoef_Array = typename CRunge_Kutta_ODE_Solver_Base<N>::TCoef_Array;
		using TCoef_Matrix = typename CRunge_Kutta_ODE_Solver_Base<N>::TCoef_Matrix;
		using TK_Coef_Array = typename CRunge_Kutta_ODE_Solver_Base<N>::TK_Coef_Array;

	private:
		const size_t mMax_stepCnt;
		// alternative weights vector (b*_i from Butcher's tableau)
		const TCoef_Array mWeights_Alt;
		// error threshold for error estimation to perform step adaptation
		const double mError_Threshold;

		// instance of step adjuster
		const Adaptive_Strategy mStep_Adjuster;

	protected:
		// Evaluates K coefficients and performs K-summation to estimate process variable difference and error of this estimation
		double Evaluate_K_Coefs(const TK_Coef_Array& kfunc, const double stepSize, double& errEstimate) const
		{
			double sumK = 0;
			double sumK_Alt = 0;
			for (size_t j = 0; j < N; j++)
			{
				sumK += CRunge_Kutta_ODE_Solver_Base<N>::mWeights[j] * kfunc[j + 1];
				sumK_Alt += mWeights_Alt[j] * kfunc[j + 1];
			}

			errEstimate = abs(sumK * stepSize - sumK_Alt * stepSize);

			return sumK * stepSize;
		}

	public:
		explicit CRunge_Kutta_ODE_Solver_Adaptive(const TCoef_Matrix& rkMatrix, const TCoef_Array& weights, const TCoef_Array& weights_alt, const TCoef_Array& nodes, const double errThreshold, const size_t max_steps)
			: CRunge_Kutta_ODE_Solver_Base<N>(rkMatrix, weights, nodes), mWeights_Alt(weights_alt), mError_Threshold(errThreshold), mMax_stepCnt(max_steps) {
		}

		template<typename _ObjFunc>
		double Step(_ObjFunc& objectiveFnc, const double T, const double X, const double stepSize) const
		{
			size_t stepCnt;
			double xDiff;
			double errEst;
			double curStep = stepSize;

			double totalRemaining = stepSize;

			double xRes = X;

			TK_Coef_Array kfunc;

			size_t max_stepCnt = 0;
			// perform N steps, stepCnt may be further adjusted during iteration (in Adjust_Step call)
			for (stepCnt = 1; stepCnt > 0; )
			{
				CRunge_Kutta_ODE_Solver_Base<N>::Evaluate_K_Func(objectiveFnc, kfunc, T + (stepSize - totalRemaining), xRes, curStep);

				xDiff = Evaluate_K_Coefs(kfunc, curStep, errEst);

				// error above threshold
				if (errEst > mError_Threshold)
				{
					// attempt to adjust step size
					if (!mStep_Adjuster.Adjust_Step(errEst, mError_Threshold, totalRemaining, curStep, stepCnt))
					{
						// if failed, no step reduction is possible and we have to accept current solution

						xRes += xDiff;
						stepCnt--;
						totalRemaining -= curStep;
					}
				}
				else // error under threshold, accept solution and attempt to reduce step size up (reduce number of steps)
				{
					xRes += xDiff;
					stepCnt--;
					totalRemaining -= curStep;

					mStep_Adjuster.Adjust_Step(errEst, mError_Threshold, totalRemaining, curStep, stepCnt);
				}

				max_stepCnt++;		
				if ((mMax_stepCnt >0) && (max_stepCnt>=mMax_stepCnt)) break;
			}			

			return xRes;
		}
};

/*
 * Adapter class interface, used just to be able to "hide" multiple classes behind disambiguation
 */
template<size_t N, bool AdaptiveStep = false,
	class Adaptive_Strategy = CRunge_Kuttta_Adaptive_Strategy_Binary_Subdivision<N>,
	class T = std::conditional_t<AdaptiveStep,
									CRunge_Kutta_ODE_Solver_Adaptive<N, Adaptive_Strategy>,
									CRunge_Kutta_ODE_Solver_NonAdaptive<N>>>
class CRunge_Kutta_ODE_Solver : public T
{
	public:
		using T::T; // inherit constructors
};
