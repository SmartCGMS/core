/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <string>
#include <vector>

void trim(std::string& s);
std::vector<std::string> split_string(std::string str, std::string delimiterOpen, std::string delimiterClose);
std::vector<std::string> split_string(std::string str, std::string delimiter);
std::vector<std::string> split_first_string(std::string str, std::string delimiter);
