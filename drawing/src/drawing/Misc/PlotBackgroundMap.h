/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
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
