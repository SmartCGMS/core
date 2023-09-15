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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <cmath>
#include <limits>

#include "gct_common.h"

/**
 * Base for all moderation functions
 */
class CModeration_Function {

	public:
		CModeration_Function() = default;
		virtual ~CModeration_Function() = default;

		/**
		 * Retrieves moderator input - coefficient of moderation
		 * The output is then multiplied by base transfer amount to obtain resulting transfer amount
		 * e.g.: if this function returns 0, transferred amount will be reduced to 0 and nothing will be transferred
		 *       if this function returns 1, transferred amount won't be affected by moderator
		 */
		virtual double Get_Moderation_Input(double moderatorAmount) const = 0;

		/**
		 * Retrieves a coefficient of moderator elimination
		 * This is the second part of transfer moderation process - what amount of moderator is processed to transfer certain
		 * substance between depots
		 * e.g.; if this function returns 0, moderator is not affected by transfer
		 */
		virtual double Get_Elimination_Input(double moderatorAmount) const = 0;
};

/**
 * Transfer is moderated in proportionally to moderator
 * Moderator does not get eliminated
 * e.g.; moderation by continuous source of external stimulation (physical exercise, ...)
 */
class CLinear_Moderation_No_Elimination_Function : public CModeration_Function {

	protected:
		double mModeration_Factor = 1.0;

	public:
		CLinear_Moderation_No_Elimination_Function(double modFactor)
			: mModeration_Factor(modFactor) {
			//
		}

		virtual double Get_Moderation_Input(double moderatorAmount) const override {
			return moderatorAmount * mModeration_Factor;
		}

		virtual double Get_Elimination_Input(double moderatorAmount) const override {
			return 0.0;
		}
};

/**
 * Transfer is moderated in proportionally to moderator with base of 1.0
 * Moderator does not get eliminated
 * e.g.; moderation of transfers, that would happen even without the presence of the moderator
 */
class CLinear_Base_Moderation_No_Elimination_Function : public CModeration_Function {

	protected:
		double mModeration_Factor = 1.0;

	public:
		CLinear_Base_Moderation_No_Elimination_Function(double modFactor)
			: mModeration_Factor(modFactor) {
			//
		}

		virtual double Get_Moderation_Input(double moderatorAmount) const override {
			return 1.0 + moderatorAmount * mModeration_Factor;
		}

		virtual double Get_Elimination_Input(double moderatorAmount) const override {
			return 0.0;
		}
};

/**
 * Transfer is moderated proportionally to moderator, starting from certain threshold
 * Moderator does not get eliminated
 * e.g.; moderation of insulin production by glucose appearance
 */
class CThreshold_Linear_Moderation_No_Elimination_Function : public CModeration_Function {

	protected:
		double mModeration_Factor = 1.0;
		double mModerator_Threshold = 0.0;

	public:
		CThreshold_Linear_Moderation_No_Elimination_Function(double modFactor, double moderatorThreshold)
			: mModeration_Factor(modFactor), mModerator_Threshold(moderatorThreshold) {
			//
		}

		virtual double Get_Moderation_Input(double moderatorAmount) const override {
			return (moderatorAmount > mModerator_Threshold) ? (moderatorAmount - mModerator_Threshold) * mModeration_Factor : 0.0;
		}

		virtual double Get_Elimination_Input(double moderatorAmount) const override {
			return 0.0;
		}
};

/**
 * Transfer is moderated proportionally to moderator
 * Moderator is eliminated proportionally to transferred amount
 * e.g.; glucose disappearance (utilization) moderated by insulin
 */
class CLinear_Moderation_Linear_Elimination_Function : public CModeration_Function {

	protected:
		double mModeration_Factor = 1.0;
		double mElimination_Factor = 1.0;

	public:
		CLinear_Moderation_Linear_Elimination_Function(double modFactor, double elimFactor)
			: mModeration_Factor(modFactor), mElimination_Factor(elimFactor) {
			//
		}

		virtual double Get_Moderation_Input(double moderatorAmount) const override {
			return moderatorAmount * mModeration_Factor;
		}

		virtual double Get_Elimination_Input(double moderatorAmount) const override {
			return moderatorAmount * mElimination_Factor;
		}
};

/**
 * Experimental: physical activity-based moderation of glucose appearance
 */
class CPA_Production_Moderation_Function : public CModeration_Function {

	protected:
		double mModeration_Factor = 1.0;

	public:
		CPA_Production_Moderation_Function(double modFactor)
			: mModeration_Factor(modFactor) {
			//
		}

		virtual double Get_Moderation_Input(double moderatorAmount) const override {
			// TODO: fix the following to better reflect reality
			return std::max(0.0, 1.5 * (moderatorAmount - 0.05) * mModeration_Factor);
		}

		virtual double Get_Elimination_Input(double moderatorAmount) const override {
			return 0.0;
		}
};
