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

#include "../Containers/Value.h"

#include <vector>
#include <array>

/** General **/

extern const std::array<TCoordinate, 9> Coords_a;
extern const std::array<TCoordinate, 4> Coords_b_up;
extern const std::array<TCoordinate, 8> Coords_b_down;
extern const std::array<TCoordinate, 3> Coords_c_up;
extern const std::array<TCoordinate, 3> Coords_c_down;
extern const std::array<TCoordinate, 5> Coords_d_up;
extern const std::array<TCoordinate, 4> Coords_d_down;
extern const std::array<TCoordinate, 4> Coords_e_up;
extern const std::array<TCoordinate, 4> Coords_e_down;

/** Parkes **/

/** diabetes 1 **/
extern const std::array<TCoordinate, 12> Coords_a_t1;
extern const std::array<TCoordinate, 10> Coords_b_up_t1;
extern const std::array<TCoordinate, 9> Coords_b_down_t1;
extern const std::array<TCoordinate, 10> Coords_c_up_t1;
extern const std::array<TCoordinate, 7> Coords_c_down_t1;
extern const std::array<TCoordinate, 8> Coords_d_up_t1;
extern const std::array<TCoordinate, 4> Coords_d_down_t1;
extern const std::array<TCoordinate, 4> Coords_e_up_t1;

/** diabetes 2 **/
extern const std::array<TCoordinate, 11> Coords_a_t2;
extern const std::array<TCoordinate, 7> Coords_b_up_t2;
extern const std::array<TCoordinate, 8> Coords_b_down_t2;
extern const std::array<TCoordinate, 7> Coords_c_up_t2;
extern const std::array<TCoordinate, 7> Coords_c_down_t2;
extern const std::array<TCoordinate, 7> Coords_d_up_t2;
extern const std::array<TCoordinate, 6> Coords_d_down_t2;
extern const std::array<TCoordinate, 4> Coords_e_up_t2;
