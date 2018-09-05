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

#include "TimeRoutines.h"
#include "../../../../common/rtl/winapi_mapping.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

bool Is_Valid_Tm(std::tm& v)
{
	// TODO: tweak this to truly validate result (i.e. day and month range, ..)
	return !(v.tm_mday == 0 || v.tm_mon < 0 || v.tm_year == 0);
}

bool Convert_Timestamp(std::string source, const char* sourceFormat, std::string &dest, const char* destFormat, time_t* unixTimeDst)
{
	std::tm convtime = {};
	std::istringstream ss(source);

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
