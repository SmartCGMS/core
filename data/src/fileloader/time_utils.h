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

#include <ctime>
#include <string>
#include <vector>


 // recognized datetime format formatter strings
class CDateTime_Detector : public std::vector<std::string> {
public:
	void finalize_pushes();
	const char* recognize(const wchar_t* str) const;
	const char* recognize(const std::string& str) const;
};

bool Str_Time_To_Unix_Time(const wchar_t* src, const char* src_fmt, std::string outFormatStr, const char* outFormat, time_t& target);
bool Str_Time_To_Unix_Time(const std::string& src, const char* src_fmt, std::string outFormatStr, const char* outFormat, time_t& target);


// Validates date conversion result
bool Is_Valid_Tm(std::tm& v);

// Converts timestamp from one format to another; returns true on success
bool Convert_Timestamp(std::string source, const char* sourceFormat, std::string &dest, const char* destFormat, time_t* unixTimeDst = nullptr);

// Converts timestamp from source format to unix timestamp
bool Convert_TimeString_To_UnixTime(std::string source, const char* sourceFormat, time_t& unixTimeDst);

// Converts unix timestamp to string in destination format
bool Convert_UnixTime_To_TimeString(time_t source, const char* destFormat, std::string& dest);
