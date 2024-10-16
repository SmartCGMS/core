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

#include "../two_signals.h"

#include <vector>
#include <array>

struct TError_Grid_Point { //everything is in mmol/L
	double expected;   //expected value aka reference value
	double calculated; //calculated value aka error value
};

enum class NError_Grid_Zone : size_t {
	A = 0,
	B = 1,
	C = 2,
	D = 3,
	E = 4,
	count,

	Undefined = std::numeric_limits<size_t>::max(),
};

struct TError_Grid_Region {
	const NError_Grid_Zone zone;				//zone into which this region belongs
	const std::vector<TError_Grid_Point> &vertices;	//an array of vertices		
};

using TError_Grid = std::vector<TError_Grid_Region>;
using TError_Grid_Stats = std::array<double, static_cast<size_t>(NError_Grid_Zone::count)>;

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CDiabetes_Grid : public virtual CTwo_Signals {
	protected:
		static NError_Grid_Zone Classify_Point(const TError_Grid& grid, double reference, double error);
		static bool Point_In_Polygon(const std::vector<TError_Grid_Point>& vertices, const double expected, const double calculated);
		static TError_Grid_Stats Calculate_Statistics(const TError_Grid& grid, std::vector<double>& reference, std::vector<double>& error, bool useRelativeCounts = true);

		virtual void Do_Flush_Stats(std::wofstream stats_file) override final;

	public:
		CDiabetes_Grid(scgms::IFilter* output);
		virtual ~CDiabetes_Grid() = default;
};

#pragma warning( pop )