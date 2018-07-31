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
