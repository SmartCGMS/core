/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <vector>
#include "Containers/Value.h"
#include "Containers/Diff2.h"
#include "Containers/Diff3.h"

std::vector<Value> diffuse2(Diff2 &param, std::vector<Value> &blood, std::vector<Value> &ist);
std::vector<Value> diffuse3(Diff3 &param, std::vector<Value> &blood, std::vector<Value> &ist);
