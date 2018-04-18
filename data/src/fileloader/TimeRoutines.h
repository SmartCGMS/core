#pragma once

#include <ctime>
#include <string>

// Validates date conversion result
bool Is_Valid_Tm(std::tm& v);

// Converts timestamp from one format to another; returns true on success
bool Convert_Timestamp(std::string source, const char* sourceFormat, std::string &dest, const char* destFormat, time_t* unixTimeDst = nullptr);

// Converts timestamp from source format to unix timestamp
bool Convert_TimeString_To_UnixTime(std::string source, const char* sourceFormat, time_t& unixTimeDst);

// Converts unix timestamp to string in destination format
bool Convert_UnixTime_To_TimeString(time_t source, const char* destFormat, std::string& dest);
