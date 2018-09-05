/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#pragma once

#include "../Containers/Value.h"

#include <vector>
#include <array>

//The array declarations are needed to prevent memory leaks, which occur when combined with TBB due to use of different allocators simultaneously

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
