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

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/SolverIface.h"

namespace internal {
	BOOL IfaceCalling Parameters_Fitness_Wrapper(const void* data, const double* solution, double* const fitness);
}

extern "C" HRESULT IfaceCalling optimize_parameters(scgms::IFilter_Chain_Configuration *configuration, const size_t filter_index, const wchar_t *parameters_configuration_name,
													scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
													const GUID *solver_id, const size_t population_size, const size_t max_generations, 
													const double** hints, const size_t hint_count,
													solver::TSolver_Progress *progress,
													refcnt::wstr_list * error_description);

extern "C" HRESULT IfaceCalling optimize_multiple_parameters(scgms::IFilter_Chain_Configuration *configuration, const size_t *filter_indices, const wchar_t **parameters_configuration_names, size_t filter_count,
													scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data,
													const GUID *solver_id, const size_t population_size, const size_t max_generations, 
													const double** hints, const size_t hint_count,
													solver::TSolver_Progress *progress,
													refcnt::wstr_list *error_description);