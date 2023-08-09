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

#include "uptake_accumulator.h"

#include <algorithm>
#undef max
#undef min

#include <iostream>
#include <limits>

void Uptake_Accumulator::Add_Uptake(double t, double t_delta_end, double amount)
{
	push_back({ t, t + t_delta_end, amount });
}

double Uptake_Accumulator::Get_Disturbance(double t_start, double t_end) const
{
	if (t_end - t_start < std::numeric_limits<double>::epsilon())
		return 0.0;

	double sum = 0;
	for (auto itr = begin(); itr != end(); ++itr)
	{
		auto& evt = *itr;

		// overlaps?
		if (t_start < evt.t_max && evt.t_min < t_end)
		{
			auto a = std::max(evt.t_min, t_start);
			auto b = std::min(evt.t_max, t_end);

			sum += evt.amount * ((b - a) / (t_end - t_start));
		}
	}

	return sum;
}

double Uptake_Accumulator::Get_Recent(double t) const {
	if (empty())
		return 0.0;

	const Uptake_Event* cur = &(*rbegin());
	for (auto itr = rbegin(); itr != rend(); ++itr)
	{
		auto &evt = *itr;
		if (t >= evt.t_min && t <= evt.t_max && cur->t_min < evt.t_min)
			cur = &evt;
	}

	return cur->amount;	
}

void Uptake_Accumulator::Cleanup(double t)
{
	for (auto itr = begin(); itr != end(); )
	{
		auto& evt = *itr;
		if (t > evt.t_max)
			itr = erase(itr);
		else
			itr++;
	}
}

void Uptake_Accumulator::Cleanup_Not_Recent(double t)
{
	if (empty())
		return;

	const Uptake_Event* cur = &(*rbegin());

	for (auto itr = rbegin(); itr != rend(); ++itr)
	{
		auto& evt = *itr;
		if (t >= evt.t_min && t <= evt.t_max && cur->t_min < evt.t_min)
			cur = &evt;
	}

	std::vector<Uptake_Event> remains;

	for (auto itr = begin(); itr != end(); itr++)
	{
		auto& evt = *itr;
		if (&evt == cur || evt.t_min >= t)
			remains.push_back(evt);
	}

	clear();
	assign(remains.begin(), remains.end());
}
