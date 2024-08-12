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

#include "diabetes_grid.h"
#include "clarke_error_grid.h"
#include "parkes_error_grid.h"

namespace {
	constexpr size_t idx_ceg = 0;
	constexpr size_t idx_peg1 = 1;
	constexpr size_t idx_peg2 = 2;
}

CDiabetes_Grid::CDiabetes_Grid(scgms::IFilter* output) : CBase_Filter(output), CTwo_Signals(output) {
}

//Classify a single point (may be external one) into the proper zone.
NError_Grid_Zone CDiabetes_Grid::Classify_Point(const TError_Grid& grid, double reference, double error) {
	for (const auto& region : grid) {
		if (Point_In_Polygon(region.vertices, reference, error)) {
			return region.zone;
		}
	}

	return NError_Grid_Zone::Undefined;
}

//Determines whether the given point (x = expected, y = calculated) lies inside (return = 1), on (return = 0)
//or outside (return = -1) the specified polygon whose vertices must be oriented CCW, N is the number of vertices.
bool CDiabetes_Grid::Point_In_Polygon(const std::vector<TError_Grid_Point>& vertices, const double expected, const double calculated) {
	//ray-crossing algorithm by PJ. Scheider and DH. Eberly. Geometric Tools for Computer Graphics. pg. 700
	bool inside = false;
	for (size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i, i++) {
		const TError_Grid_Point& A = vertices[i];
		const TError_Grid_Point& B = vertices[j];

		if (calculated < B.calculated) {
			//B is above the horizontal ray passing through P (expected, calculated)
			if (A.calculated <= calculated) {
				//A is below or on the horizontal ray => there is some intersection
				//if the intersection is to the right from P, change the status, otherwise ignore it
				if ((calculated - A.calculated) * (B.expected - A.expected) > (expected - A.expected) * (B.calculated - A.calculated)) {
					inside = !inside;
				}
			}
		}
		else if (calculated < A.calculated) {
			//B is below or on the horizontal ray => there is some intersection
			//if the intersection is to the right from P, take it, otherwise ignore it
			//n.b., the different sign is here because to avoid division we multiplied both sides of inequation by the denominator
			//which in this case is negative, i.e., it turns the sign
			if ((calculated - A.calculated) * (B.expected - A.expected) < (expected - A.expected) * (B.calculated - A.calculated)) {
				inside = !inside;
			}
		}
	}

	return inside;
}

//Counts the number of points in every zone. If useRelativeCounts is true, the counted values are normalized so that their sum is 1.0.
TError_Grid_Stats CDiabetes_Grid::Calculate_Statistics(const TError_Grid& grid, std::vector<double>& reference, std::vector<double>& error,  bool useRelativeCounts) {
	TError_Grid_Stats stats;

	std::fill(stats.begin(), stats.end(), 0.0);

	if (reference.empty() || error.empty() || (reference.size() != error.size())) {
		std::fill(stats.begin(), stats.end(), std::numeric_limits<double>::quiet_NaN());
		return stats;
	}

	size_t numValid = 0;
	for (size_t i = 0; i < reference.size(); i++) {	
		const auto zone = Classify_Point(grid, reference[i], error[i]);

		if (zone != NError_Grid_Zone::Undefined) {
			stats[static_cast<size_t>(zone)] += 1.0;
			++numValid;
		}
	}

	if (useRelativeCounts)	{
		const double invSize = 1.0 / numValid;
		double sum = 0.0;
		for (size_t i = 1; i < static_cast<size_t>(NError_Grid_Zone::count); i++) {
			stats[i] = stats[i] * invSize;
			sum += stats[i];
		}

		stats[0] = 1.0 - sum;	//make sure we have 1.0 "exactly"
	}

	return stats;
}

void CDiabetes_Grid::Do_Flush_Stats(std::wofstream stats_file) {
	struct TAll_Grids {
		uint64_t segment_id;
		std::array<TError_Grid_Stats, 3> grids;
	};

	std::vector<TAll_Grids > grids;

	{ //use a block to avoid re-allocations
		std::vector<double> reference_times, reference_levels, error_levels;
		auto get_segment = [this, &grids, &reference_times, &reference_levels, &error_levels](const uint64_t segment_id) {

			if (Prepare_Levels(segment_id, reference_times, reference_levels, error_levels)) {
				TAll_Grids segment_grids;
				segment_grids.segment_id = segment_id;
				segment_grids.grids[idx_ceg] = Calculate_Statistics(Clarke_Error_Grid, reference_levels, error_levels, true);
				segment_grids.grids[idx_peg1] = Calculate_Statistics(Parkes_Error_Grid_Type_1, reference_levels, error_levels, true);
				segment_grids.grids[idx_peg2] = Calculate_Statistics(Parkes_Error_Grid_Type_2, reference_levels, error_levels, true);

				grids.push_back(segment_grids);
			}
		};

		//1. obtain all the statistics so that we can avoid mixing segments and different error grids together
		for (auto& signals: mSignal_Series) {
			get_segment(signals.first);
		}
		get_segment(scgms::All_Segments_Id);
	}

	//2. write the error grids
	const std::array<const wchar_t*, 3> markers = { dsClarke_Error_Grid, dsParkes_Error_Grid_Type_1, dsParkes_Error_Grid_Type_2 };

	auto flush_stats = [&stats_file](const TError_Grid_Stats& stats, const uint64_t segment_id) {

		if (segment_id == scgms::All_Segments_Id) {
			stats_file << dsSelect_All_Segments;
		}
		else {
			stats_file << std::to_wstring(segment_id);
		}

		stats_file << ";; " << stats[0] << "; " << stats[1] << "; " << stats[0]+stats[1] << "; " << stats[2] << "; " << stats[3] << "; " << stats[4] << "; " << std::endl;
	};
	
	for (size_t idx = idx_ceg; idx <= idx_peg2; idx++) {
		stats_file << markers[idx] << std::endl;
		stats_file << rsDiabetes_Grid_Header << std::endl;
		for (const auto& segment_grid : grids) {
			flush_stats(segment_grid.grids[idx], segment_grid.segment_id);
		}

		stats_file << std::endl << std::endl;
	}
}
