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

#include "depot.h"

void CDepot_Link::Step(const double currentTime) {
	// Transfer amount from source to target
	// If the target refuses to accept given amount, return it back to source

	// Note that precision is not a subject of matter here - the larger the step, the worse
	// is the precision; so it depends completely on outer code

	// the transfer hasn't started yet (delayed transfer)
	if (!mTransfer_Function->Is_Started(currentTime)) {
		return;
	}

	// no origin point in time, set given time as origin and return
	if (std::isnan(mLast_Time)) {
		mLast_Time = currentTime;
		return;
	}

	// Simpson's 1/3 rule for integration

	const double y_diff = currentTime - mLast_Time;
	const double x0 = mTransfer_Function->Calculate_Transfer_Input(mLast_Time);
	const double x1 = mTransfer_Function->Calculate_Transfer_Input((mLast_Time + currentTime) / 2.0);
	const double x2 = mTransfer_Function->Calculate_Transfer_Input(currentTime);

	// TODO: this may have a little rework later, so we don't have to return NaN for "no change in logic"
	double baseAmount = mTransfer_Function->Get_Transfer_Amount();
	if (std::isnan(baseAmount)) {
		baseAmount = mSource.get().Get_Quantity();
	}

	double amount = - (y_diff / 6.0) * (x0 + 4 * x1 + x2) * baseAmount;

	// transfer moderation
	for (const auto& mods : mModerators) {

		amount *= mods.function.get()->Get_Moderation_Input(mods.depot.get().Get_Quantity());
		double eliminateAmount = -mods.function.get()->Get_Elimination_Input(mods.depot.get().Get_Quantity());
		mods.depot.get().Mod_Quantity(eliminateAmount);
	}

	// NOTE: source and destination is selected dynamically - this is for e.g.; two-way links like diffusion
	auto& src = (amount < 0.0) ? mSource.get() : mTarget.get();
	auto& dst = (amount < 0.0) ? mTarget.get() : mSource.get();
	amount = -abs(amount);

	src.Mod_Quantity(amount);	// modifies amount in source compartment; 'amount' is modified
	const double result_1 = amount;

	dst.Mod_Quantity(amount);	// modifies amount in destination compartment; 'amount' is modified
	const double result_2 = amount;

	// dst was unable to accept/give all amount, return it back
	if (result_1 != -result_2) {
		src.Return_Quantity(-result_2);
	}

	mLast_Time = currentTime;
}
