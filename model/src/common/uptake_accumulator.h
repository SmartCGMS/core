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

#include <vector>

// single uptake event to be taken into account
struct Uptake_Event
{
	double t_min;		// device time of start
	double t_max;		// device time offset of end from start
	double amount;		// [UNITS/min] - unit could be anything (U, g, ...)
};

// container of uptake events
class Uptake_Accumulator : public std::vector<Uptake_Event>
{
	public:
		// adds a new uptake record
		void Add_Uptake(double t, double t_delta_end, double amount);

		// retrieves sum of all disturbaces at given time point
		double Get_Disturbance(double t) const;

		// retrieves the most recent disturbance at given time point
		double Get_Recent(double t) const;

		// cleans up everything that has taken place before given time point
		void Cleanup(double t);

		// cleans up everything that is not a recent record
		void Cleanup_Not_Recent(double t);
};
