#pragma once

#include <string>
#include <ctime>

std::time_t utc_time();
std::wstring to_iso8601(const std::time_t &timestamp, const bool include_timezone = true);
std::time_t from_iso8601(const std::wstring &descriptor);
