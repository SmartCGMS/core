/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include <ctime>
#include <cmath>

// define localtime function depending on compiler to avoid unnecessary warnings
#ifdef _MSC_VER
#define _localtime(pTm, tval) localtime_s(pTm, &tval)
#else
#define _localtime(pTm, tval) pTm = localtime(&tval)
#endif

// how many seconds are there in a day
constexpr time_t SECONDS_IN_DAY = 24 * 60 * 60;
