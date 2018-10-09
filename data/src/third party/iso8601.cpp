#include "iso8601.h"
#include "../../../../common/utils/winapi_mapping.h"

//https://github.com/logandk/restful_mapper/blob/master/include/restful_mapper/internal/iso8601.h

/*
	Copyright(c) 2013, Logan Raarup
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met :

	*Redistributions of source code must retain the above copyright notice, this
		list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the documentation
		and/or other materials provided with the distribution.
	* The names of the contributors may not be used to endorse or promote products
		derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cctype>
#include <time.h>

/**
 * @brief Calculate the current UTC time stamp
 *
 * @return the UTC time stamp
 */
std::time_t utc_time() {
	std::time_t now = std::time(NULL);

	std::tm tm_local;
	localtime_s(&tm_local, &now);
	std::time_t t_local = std::mktime(&tm_local);

	std::tm tm_utc;  
	gmtime_s(&tm_utc, &t_local);
	std::time_t t_utc = std::mktime(&tm_utc);

	return now - (t_utc - t_local);
}

/**
 * @brief Produces an ISO-8601 string representation of the timestamp
 *
 * @param timestamp the epoch time in seconds
 * @param include_timezone appends Z UTC flag at end of string if true
 *
 * @return a string representing the timestamp in UTC
 */
std::wstring to_iso8601(const std::time_t &timestamp, const bool include_timezone) {
	std::tm exploded_time; 
	gmtime_s(&exploded_time, &timestamp);
	std::wstring ret;

	if (include_timezone)
	{
		wchar_t buf[sizeof L"1970-01-01T00:00:00Z"];
		std::wcsftime(buf, sizeof buf, L"%Y-%m-%dT%H:%M:%SZ", &exploded_time);
		ret = buf;
	}
	else
	{
		wchar_t buf[sizeof L"1970-01-01T00:00:00"];
		std::wcsftime(buf, sizeof buf, L"%Y-%m-%dT%H:%M:%S", &exploded_time);
		ret = buf;
	}

	return ret;
}

/**
 * @brief Parses an ISO-8601 formatted string into epoch time
 *
 * @param descriptor the ISO-8601
 *
 * @return the UTC timestamp
 */
std::time_t from_iso8601(const std::wstring &descriptor) {
	// Parse according to ISO 8601
	std::tm t;

	const wchar_t *value = descriptor.c_str();
	const wchar_t *c = value;
	int year = 0;
	int month = 0;
	int seconds = 0;
	int minutes = 0;
	int hours = 0;
	int days = 0;

	// NOTE: we have to check for the extended format first,
	// because otherwise the separting '-' will be interpreted
	// by std::sscanf as signs of a 1 digit integer .... :-(
	if (swscanf_s(value, L"%4u-%2u-%2u", &year, &month, &days) == 3)
	{
		c += 10;
	}
	else if (swscanf_s(value, L"%4u%2u%2u", &year, &month, &days) == 3)
	{
		c += 8;
	}
	else
	{
		return 0;
		//throw std::runtime_error(std::string("Invalid date format: ") + value);
	}

	t.tm_year = year - 1900;
	t.tm_mon = month - 1;
	t.tm_mday = days;

	// Check if time is supplied
	if (*c == '\0')
	{
		t.tm_hour = 0;
		t.tm_sec = 0;
		t.tm_min = 0;
	}
	else if (*c == 'T')
	{
		// Time of day part
		c++;

		if (swscanf_s(c, L"%2d%2d", &hours, &minutes) == 2)
		{
			c += 4;
		}
		else if (swscanf_s(c, L"%2d:%2d", &hours, &minutes) == 2)
		{
			c += 5;
		}
		else
		{
			return 0; // throw std::runtime_error(std::string("Invalid date format: ") + value);
		}

		if (*c == ':')
		{
			c++;
		}

		if (*c != '\0')
		{
			if (swscanf_s(c, L"%2d", &seconds) == 1)
			{
				c += 2;
			}
			else
			{
				return 0; // throw std::runtime_error(std::string("Invalid date format: ") + value);
			}
		}

		t.tm_hour = hours;
		t.tm_min = minutes;
		t.tm_sec = seconds;
	}
	else
	{
		return 0; // throw std::runtime_error(std::string("Invalid date format: ") + value);
	}

	// Drop microsecond information
	if (*c == '.')
	{
		c++;

		while (std::isdigit(*c) && *c != '\0')
		{
			c++;
		}
	}

	// Parse time zone information
	/*int*/ time_t tz_offset = 0;

	if (*c == 'Z')
	{
		c++;
	}

	if (*c != '\0')
	{
		int tz_direction = 0;

		if (*c == '+')
		{
			tz_direction = 1;
		}
		else if (*c == '-')
		{
			tz_direction = -1;
		}
		else
		{
			return 0;  //throw std::runtime_error(std::string("Invalid date format: ") + value);
		}

		// Offset part
		int tz_hours = 0;
		int tz_minutes = 0;

		c++;

		if (swscanf_s(c, L"%2d", &tz_hours) == 1)
		{
			c += 2;
		}
		else
		{
			return 0;  //throw std::runtime_error(std::string("Invalid date format: ") + value);
		}

		if (*c == ':')
		{
			c++;
		}

		if (*c != '\0')
		{
			if (swscanf_s(c, L"%2d", &tz_minutes) == 1)
			{
				c += 2;
			}
			else
			{
				return 0; // throw std::runtime_error(std::string("Invalid date format: ") + value);
			}
		}

		tz_offset = tz_direction * (tz_hours * 3600 + tz_minutes * 60);
	}

	// Determine DST automatically
	t.tm_isdst = -1;

	// Correct for local time zone
	std::time_t t_local = std::mktime(&t);
	std::tm tm_utc;// (*std::gmtime(&t_local));
	gmtime_s(&tm_utc, &t_local);
	std::time_t t_utc = std::mktime(&tm_utc);
	tz_offset += (t_utc - t_local);

	return std::mktime(&t) - tz_offset;
}

