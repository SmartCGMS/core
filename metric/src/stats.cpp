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

#include "stats.h"

#include <cmath>
#include <numeric>
#include <algorithm>

void Set_Error_To_No_Data(scgms::TSignal_Stats& signal_error) {
	signal_error.count = 0;
	signal_error.sum = 0.0;
	signal_error.avg = std::numeric_limits<double>::quiet_NaN();
	signal_error.stddev = std::numeric_limits<double>::quiet_NaN();

	const size_t ECDF_offset = static_cast<size_t>(scgms::NECDF::min_value);
	for (size_t i = 0; i <= static_cast<size_t>(scgms::NECDF::max_value) - ECDF_offset; i++)
		signal_error.ecdf[ECDF_offset + i] = std::numeric_limits<double>::quiet_NaN();
}

bool Calculate_Signal_Stats(std::vector<double>& series, scgms::TSignal_Stats& signal_error) {
	//1. evaluate the count
	signal_error.count = series.size();
	if (signal_error.count == 0) {
		Set_Error_To_No_Data(signal_error);
		return false;
	}


	//2. calculate ECDF
	std::sort(series.begin(), series.end());

	//fill min and max precisely as we will be rounding for the other values
	signal_error.ecdf[0] = series[0];
	signal_error.ecdf[static_cast<size_t>(scgms::NECDF::max_value)] = series[series.size() - 1];

	const double stepping = static_cast<double>(signal_error.count - 1) / (static_cast<double>(scgms::NECDF::max_value) + 1.0);
	const size_t ECDF_offset = static_cast<size_t>(scgms::NECDF::min_value);
	for (size_t i = 1; i < static_cast<size_t>(scgms::NECDF::max_value) - ECDF_offset; i++)
		signal_error.ecdf[i + ECDF_offset] = series[static_cast<size_t>(std::round(static_cast<double>(i)* stepping))];


	//3. calculate count, sum and avg
	double corrected_count = static_cast<double>(signal_error.count);
		
	signal_error.sum = std::accumulate(series.begin(), series.end(), 0.0);
	signal_error.avg = signal_error.sum / corrected_count;

	//3. calculate stddev
	{		
		double square_error = 0.0;
		for (const auto& value : series) {
			const double tmp = value - signal_error.avg;
			square_error += tmp * tmp;
		}

		if (corrected_count > 1.5) corrected_count -= 1.5;					//Unbiased estimation of standard deviation
			else if (corrected_count > 1.0) corrected_count -= 1.0;			//Bessel's correction

		signal_error.stddev = sqrt(square_error / corrected_count);
	}
	
	return true;
}
