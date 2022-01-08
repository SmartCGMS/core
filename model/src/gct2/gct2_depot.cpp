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

#include "gct2_depot.h"

namespace gct2_model
{
	void CDepot_Link::Step(const double currentTime) {
		// Transfer amount from source to target
		// If the target refuses to accept given amount, return it back to source

		// Note that precision is not a subject of matter here - the larger the step, the worse
		// is the precision; so it depends completely on outer code

		const CTransfer_Function& transfer_fnc = *mTransfer_Function;

		// the transfer hasn't started yet (delayed transfer)
		if (!transfer_fnc.Is_Started(currentTime)) {
			mLast_Time = currentTime;
			return;
		}

		// cache start and end points
		const auto start = transfer_fnc.Get_Start_Time();
		const auto end = transfer_fnc.Get_End_Time();

		// move time boundaries to not extrapolate during the integration
		const double fnc_past_time = std::max(mLast_Time, start);
		const double fnc_future_time = currentTime > end ? end : currentTime;

		// not in std::abs on purpose - in case the times got somehow mixed up
		if (fnc_future_time - fnc_past_time < std::numeric_limits<double>::epsilon())
			return;

		// Simpson's 1/3 rule for integration

		const double y_diff = fnc_future_time - fnc_past_time;
		const double x0 = transfer_fnc.Calculate_Transfer_Input(fnc_past_time);
		const double x1 = transfer_fnc.Calculate_Transfer_Input((fnc_past_time + fnc_future_time) / 2.0);
		const double x2 = transfer_fnc.Calculate_Transfer_Input(fnc_future_time);

		double baseAmount = transfer_fnc.Get_Transfer_Amount(mSource.get().Get_Quantity());

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
		amount = -std::fabs(amount);

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

}
