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

/** General **/

extern const std::vector<Coordinate> Coords_a;
extern const std::vector<Coordinate> Coords_b_up;
extern const std::vector<Coordinate> Coords_b_down;
extern const std::vector<Coordinate> Coords_c_up;
extern const std::vector<Coordinate> Coords_c_down;
extern const std::vector<Coordinate> Coords_d_up;
extern const std::vector<Coordinate> Coords_d_down;
extern const std::vector<Coordinate> Coords_e_up;
extern const std::vector<Coordinate> Coords_e_down;

/** Parkes **/

/** diabetes 1 **/
extern const std::vector<Coordinate> Coords_a_t1;
extern const std::vector<Coordinate> Coords_b_up_t1;
extern const std::vector<Coordinate> Coords_b_down_t1;
extern const std::vector<Coordinate> Coords_c_up_t1;
extern const std::vector<Coordinate> Coords_c_down_t1;
extern const std::vector<Coordinate> Coords_d_up_t1;
extern const std::vector<Coordinate> Coords_d_down_t1;
extern const std::vector<Coordinate> Coords_e_up_t1;

/** diabetes 2 **/
extern const std::vector<Coordinate> Coords_a_t2;
extern const std::vector<Coordinate> Coords_b_up_t2;
extern const std::vector<Coordinate> Coords_b_down_t2;
extern const std::vector<Coordinate> Coords_c_up_t2;
extern const std::vector<Coordinate> Coords_c_down_t2;
extern const std::vector<Coordinate> Coords_d_up_t2;
extern const std::vector<Coordinate> Coords_d_down_t2;
extern const std::vector<Coordinate> Coords_e_up_t2;
