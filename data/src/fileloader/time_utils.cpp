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

#include "time_utils.h"

#include "../../../../common/utils/winapi_mapping.h"
#include "../../../../common/utils/string_utils.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

void CDateTime_Detector::finalize_pushes() {

	//let us perform longest-prefix match first, just like with IP mask
	std::sort(begin(), end(), [](const std::string a, const std::string b) {	//no reference, because it moves the object!
		return a.size() > b.size();
		});

}

const char* CDateTime_Detector::recognize(const wchar_t* str) const {
	char* result = nullptr;
	if ((str != nullptr) && (*str != 0)) {
		std::string date_time_str{ Narrow_WChar(str) };
		return recognize(date_time_str);
	}

	return result;
}

const char* CDateTime_Detector::recognize(const std::string& str) const {
	size_t i;
	/*std::tm temp;
	memset(&temp, 0, sizeof(std::tm));
	*/

	// TODO: slightly rework this method to safely check for parse failure even in debug mode

	for (i = 0; i < size(); i++) {	//no for each, to traverse from longest prefix to shortest one
		time_t temp;
		std::string dst;
		const auto& mask_candidate = operator[](i);
		// is conversion result valid? if not, try next line
		if (Str_Time_To_Unix_Time(str, mask_candidate.c_str(), dst, nullptr, temp))
			return mask_candidate.c_str();
	}

	return nullptr;
}

bool Str_Time_To_Unix_Time(const wchar_t* src, const char* src_fmt, std::string outFormatStr, const char* outFormat, time_t& target) {
	if (src == nullptr) return false;
	std::string tmp{ Narrow_WChar(src) };
	return Str_Time_To_Unix_Time(tmp, src_fmt, outFormatStr, outFormat, target);
}

bool Str_Time_To_Unix_Time(const std::string& src, const char* src_fmt, std::string outFormatStr, const char* outFormat, time_t& target) {
	return Convert_Timestamp(src, src_fmt, outFormatStr, outFormat, &target);
}

bool Is_Valid_Tm(std::tm& v)
{
	// TODO: tweak this to truly validate result (i.e. day and month range, ..)
	return !(v.tm_mday == 0 || v.tm_mon < 0 || v.tm_year == 0);
}

bool Convert_Timestamp(std::string source, const char* sourceFormat, std::string &dest, const char* destFormat, time_t* unixTimeDst)
{
	std::tm convtime = {};
	std::istringstream ss(source + " "); //we append extra space to avoid 1b) and enforce 1c) for https://en.cppreference.com/w/cpp/locale/time_get/get
										 //practically, if mask would e.g.; contain seconds, but the source would not, then the conversion would still succeed despite missing seconds
										 //the trailing, extra space removes the eof, thus making the conversion fail for the missing seconds

	if (ss >> std::get_time(&convtime, sourceFormat))
	{
		if (!Is_Valid_Tm(convtime))
			return false;

		if (destFormat)
		{
			std::stringstream timestr;
			timestr << std::put_time(&convtime, destFormat);

			dest = timestr.str();
		}

		if (unixTimeDst)
			*unixTimeDst = mktime(&convtime);

		return true;
	}
	else
		return false;
}

bool Convert_TimeString_To_UnixTime(std::string source, const char* sourceFormat, time_t& unixTimeDst)
{
	std::tm convtime = {};
	std::istringstream ss(source);

	if (ss >> std::get_time(&convtime, sourceFormat))
	{
		if (!Is_Valid_Tm(convtime))
			return false;

		unixTimeDst = mktime(&convtime);

		return true;
	}
	else
		return false;
}

bool Convert_UnixTime_To_TimeString(time_t source, const char* destFormat, std::string& dest)
{
	std::tm convtime;
	localtime_s(&convtime, &source);

	if (!Is_Valid_Tm(convtime))
		return false;

	std::stringstream timestr;
	timestr << std::put_time(&convtime, destFormat);

	dest = timestr.str();

	return true;
}
