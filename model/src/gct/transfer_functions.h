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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include <cmath>
#include <limits>

#include "common.h"

class CTransfer_Function;

/**
 * Base for all transfer functions between two depots
 */
class CTransfer_Function {

	public:
		// constant for marking "beginning of time"
		static constexpr double Start = 0.0;
		// constant for marking "no limit for this transfer function"
		static constexpr double Unlimited = std::numeric_limits<double>::max();

	private:
		// when did this transfer function start? For continuous transfer, use CTransfer_Function::Start
		const double mTime_Start = Start;
		// how long should this transfer function last? For continuous transfer, use CTransfer_Function::Unlimited
		const double mDuration = Unlimited;

	protected:
		/**
		 * Retrieves scaled time from simulation-time to the interval <0;1>
		 */
		double Get_Scaled_Time(double currentTime) const {
			return (currentTime - mTime_Start) / mDuration;
		}

		/**
		 * Get scaling factor for scaling the function value
		 */
		double Get_Amount_Scaling_Factor() const {
			return 1.0 / mDuration;
		}

	public:
		CTransfer_Function(double timeStart = 0.0, double duration = std::numeric_limits<double>::max()) noexcept : mTime_Start(timeStart), mDuration(duration) {
		}

		// rule-of-zero - let outer container maintain this resource, as it's likely to be a 1:1 link

		// prohibit copy
		CTransfer_Function(const CTransfer_Function& other) = delete;
		CTransfer_Function& operator=(const CTransfer_Function& other) = delete;
		// prohibit move
		CTransfer_Function(CTransfer_Function&& other) = delete;
		CTransfer_Function& operator=(CTransfer_Function&& other) = delete;

		virtual ~CTransfer_Function() = default;

		/**
		 * Calculates the factor of transfer at time 'time'
		 * 
		 * Integral of fixed-size dose absorptions should be 1 to preserve masses
		 * Other transfers (such as continuously refreshed depots) have no restrictions
		 */
		virtual double Calculate_Transfer_Input(double time) const = 0;

		/**
		 * Transfer function may specify transfer amount; by default, outer code does that; indicate this by returning NaN
		 */
		virtual double Get_Transfer_Amount() const {
			return std::numeric_limits<double>::quiet_NaN();
		}

		/**
		 * Is this transfer function over? i.e.; it transfered everything it could transfer and is a subject to deletion
		 */
		bool Is_Expired(double time) const {
			return time > mTime_Start + mDuration;
		}

		/**
		 * Has the transfer begin yet? this may come in handy if we plan some "inevitable" activity, like eating a meal "one bite per X seconds"
		 */
		bool Is_Started(double time) const {
			return time >= mTime_Start;
		}
};

/**
 * Base for all bounded transfer functions; preserves initial amount, so exactly this amount is transfered
 * Integral of all bounded functions must be equal to 1, so sum of transferred amounts must give the overall (initial) amount
 */
class CBounded_Transfer_Function : public CTransfer_Function {

	protected:
		// this much of a substance is present on the beginning of the transfer
		const double mInitial_Amount = 0.0;

	public:
		CBounded_Transfer_Function(double timeStart, double duration, double initialAmount)
			: CTransfer_Function(timeStart, duration), mInitial_Amount(initialAmount) {
			//
		}

		virtual double Get_Transfer_Amount() const override {
			return mInitial_Amount;
		}
};

/**
 * Constant unbounded transfer function
 */
class CConstant_Unbounded_Transfer_Function final : public CTransfer_Function {

	private:
		// this much of a substance is transferred each time unit
		double mTransfer_Factor = 1.0;

	public:
		CConstant_Unbounded_Transfer_Function(double timeStart, double duration, double transferFactor = 1.0)
			: CTransfer_Function(timeStart, duration), mTransfer_Factor(transferFactor) {
			//
		}

		double Calculate_Transfer_Input(double time) const override {
			return mTransfer_Factor;
		}
};

/**
 * Two-way diffusion unbounded transfer function
 */
class CTwo_Way_Diffusion_Unbounded_Transfer_Function final : public CTransfer_Function {

	private:
		// this much of a substance is transferred each time unit
		double mTransfer_Factor = 1.0;

		IQuantizable& mSource;
		IQuantizable& mTarget;

	public:
		CTwo_Way_Diffusion_Unbounded_Transfer_Function(double timeStart, double duration, IQuantizable& source, IQuantizable& target, double transferFactor = 1.0)
			: CTransfer_Function(timeStart, duration), mTransfer_Factor(transferFactor), mSource(source), mTarget(target) {
			//
		}

		double Get_Transfer_Amount() const override {
			// diffusion always transfers from depot with higher concentration
			return (mSource.Get_Concentration() > mTarget.Get_Concentration()) ? mSource.Get_Quantity() : mTarget.Get_Quantity();
		}

		double Calculate_Transfer_Input(double time) const override {

			// negative difference does not matter, the outer code manages that well (in fact in other direction)
			const double diff = (mSource.Get_Concentration() - mTarget.Get_Concentration());

			// TODO: some Michaelis-Menten wouldn't hurt
			return mTransfer_Factor * diff;
		}
};

/**
 * Concentration threshold disappearance unbounded transfer function
 */
class CConcentration_Threshold_Disappearance_Unbounded_Transfer_Function final : public CTransfer_Function {

	private:
		double mTransfer_Factor = 1.0;
		double mThreshold;

		IQuantizable& mSource;

	public:
		CConcentration_Threshold_Disappearance_Unbounded_Transfer_Function(double timeStart, double duration, IQuantizable& source, double threshold, double transferFactor = 1.0)
			: CTransfer_Function(timeStart, duration), mTransfer_Factor(transferFactor), mSource(source), mThreshold(threshold) {
			//
		}

		double Calculate_Transfer_Input(double time) const override {

			const double diff = (mSource.Get_Concentration() - mThreshold);

			// do not transfer if concentration is under threshold
			if (diff <= 0.0)
				return 0.0;

			return mTransfer_Factor * diff;
		}
};

/**
 * Basal appearance unbounded transfer function
 */
class CDifference_Unbounded_Transfer_Function final : public CTransfer_Function {

	private:
		double mTransfer_Factor = 1.0;

		IQuantizable& mSource;
		IQuantizable& mTarget;

	public:
		CDifference_Unbounded_Transfer_Function(double timeStart, double duration, IQuantizable& source, IQuantizable& target, double transferFactor = 1.0)
			: CTransfer_Function(timeStart, duration), mTransfer_Factor(transferFactor), mSource(source), mTarget(target) {
			//
		}

		double Calculate_Transfer_Input(double time) const override {

			const double diff = (mSource.Get_Quantity() - mTarget.Get_Quantity());

			// only appearance when the target level is lower
			if (diff < 0)
				return 0.0;

			return mTransfer_Factor * diff;
		}
};

/**
 * Constant bounded transfer function
 * 
 * |------------------| c
 * |                  |
 * ....................
 * t_0              t_1
 * 
 * The "c" constant is recalculated to produce an integral of 1 on <t_0 ; t_1> interval
 */
class CConstant_Bounded_Transfer_Function final : public CBounded_Transfer_Function {

	public:
		using CBounded_Transfer_Function::CBounded_Transfer_Function;

		double Calculate_Transfer_Input(double time) const override {
			return 1.0 * Get_Amount_Scaling_Factor();
		}
};

/**
 * Triangular bounded transfer function
 * 
 *           . y_p
 *         .   .
 *       .       .
 *     .           .
 *   .               .
 * ....................
 * t_0      t_p      t_1
 * 
 * y_p constant is a peak absorption point
 */
class CTriangular_Bounded_Transfer_Function final : public CBounded_Transfer_Function {

	public:
		using CBounded_Transfer_Function::CBounded_Transfer_Function;

		double Calculate_Transfer_Input(double time) const override {
			const double scTime = Get_Scaled_Time(time);

			// t < t_p
			const bool prePeak = (scTime <= 0.5);

			// NOTE: the function is already scaled to unit dimensions
			const double k = prePeak ? 4.0 : -4.0;
			const double q = prePeak ? 0   : 4.0;

			// triangular function scaled to give integral of 1.0
			const double amount = k * scTime + q;

			return amount * Get_Amount_Scaling_Factor();
		}
};