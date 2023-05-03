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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "../general.h"
#include "Utility.h"

#include <iomanip>
#include <stdexcept>

namespace Utility
{
	const std::array<const char*, 5> Curve_Colors = { { "orange", "green", "cyan", "magenta", "purple" } };

    double Time_To_Double(time_t time)
    {
        return difftime(time, 0);
    }

    int Find_Ist_Index(double date, ValueVector &ist)
    {
        for (size_t i = 1; i < ist.size(); i++)
        {
            Value& v = ist[i-1];
            Value& next = ist[i];
            if ((fabs(v.date - date) < std::numeric_limits<double>::epsilon()) || ((v.date < date) && (date < next.date)))
                return (int)i;
        }

        return -1;
    }

    double Find_Ist_Level(double date, ValueVector &ist)
    {
        int index = Find_Ist_Index(date, ist);
        if (index < 0)
            return (double)index;

        Value v = ist.at(index);
        if (v.date == date)
            return v.value;

        // approximate
        Value& next = ist[index + 1];
        double a = (next.value - v.value) / (double)(next.date - v.date);
        double b = v.value - a*(double)v.date;

        return a*date + b;
    }

	std::vector<time_t> Get_Times(time_t minDate, time_t maxDate)
	{
		// an hour before first date
	#ifdef _WIN32
		tm tim;
		_localtime(&tim, minDate);
		tm* midnight_before = &tim;
	#else
		tm* midnight_before = nullptr;
		_localtime(midnight_before, minDate);
	#endif

        if (!midnight_before)
            return {};

		midnight_before->tm_min = 0;
		midnight_before->tm_sec = 0;
		time_t before = mktime(midnight_before);

		// an hour before last date
	#ifdef _WIN32
		_localtime(&tim, maxDate);
		tm* midnight_after = &tim;
	#else
		tm* midnight_after = nullptr;
		_localtime(midnight_after, maxDate);
	#endif

        if (!midnight_after)
            return {};

		midnight_after->tm_min = 0;
		midnight_after->tm_sec = 0;
		time_t after = mktime(midnight_after);

		time_t skip = 3600; // 1 hour is the minimal default skip
		// maximum of 8 labels
		if (after - before > 8*3600)
		{
			// ensure maximum of 8 labels
			skip = ((after - before) / 8);
			// round to whole hours
			skip = skip - skip % 3600;
		}

		std::vector<time_t> times;
		for (time_t cur = before; cur <= after; cur += skip)
			times.push_back(cur);

		return times;
	}

    Value Get_Bezier_Value(Value& p0, Value& p1, Value& p2, Value& search)
    {
		const auto date_diff = search.date - p0.date;
		if (date_diff == 0) return p0.value;

        double t = Time_To_Double((search.date - p0.date) / date_diff);
        double y1 = p0.value*(1 - t)*(1 - t) + 2 * t*p1.value*(1 - t) + t*t*p2.value;
        Value v;
        v.date = search.date;
        v.value = y1;
        return v;
    }

    void Get_Boundary_Dates(ValueVector &istVector, ValueVector &bloodVector, time_t &min_date, time_t &max_date)
    {
        time(&min_date); // setCurrent time
        time_t ist_min_date = istVector.empty() ? min_date : istVector.at(0).date;
        time_t ist_max_date = istVector.empty() ? 0 : istVector.at(istVector.size() - 1).date;
        time_t blood_min_date = bloodVector.empty() ? min_date : bloodVector.at(0).date;
        time_t blood_max_date = bloodVector.empty() ? 0 : bloodVector.at(bloodVector.size() - 1).date;
        min_date = ist_min_date < blood_min_date ? ist_min_date : blood_min_date;
        max_date = ist_max_date > blood_max_date ? ist_max_date : blood_max_date;
    }

    double Get_Quartile(double index, std::vector<double> &vector)
    {
        double intPart, fractpart;
        fractpart = modf(index, &intPart);
        size_t in = (size_t)intPart;

        if (fabs(fractpart) < std::numeric_limits<double>::epsilon())
            return vector.at(in);

        if (in < vector.size() - 1)
            return (vector[in] + vector[in + 1]) / 2.0;

        return vector[in];
    }

    bool Find_Value(Value &searchValue, Value &measuredBlood, ValueVector &diffVal)
    {
        if (diffVal.empty())
            return false;

        // finds nearest index from supplied date
        int index = Find_Ist_Index(Time_To_Double(measuredBlood.date), diffVal);

        if (index == -1)
            return false;

		size_t s_index = static_cast<size_t>(index);

        int indexShift = 0;

        Value& countedDiff = diffVal[s_index];
        // exact match - no need to interpolate
        if (measuredBlood.date == countedDiff.date)
            searchValue = countedDiff;
        else // otherwise approximate
        {
            // border value - no data
            if (s_index == 0 || s_index >= diffVal.size()-1)
                return false;

            // even index - bezier curve passes through this point
            if (s_index % 2 != 0)
                indexShift = 1;

            Value &p2 = diffVal[s_index + indexShift];
            Value &p1 = diffVal[s_index + indexShift - 1];
            Value &p0 = diffVal[s_index + indexShift - 2];

            searchValue = Get_Bezier_Value(p0, p1, p2, measuredBlood);
        }

        return true;
    }

    Data Get_Data_Item(DataMap &vectors, std::string key)
    {
        std::map<std::string, Data>::iterator search = vectors.find(key);
        if (search != vectors.end())
            return search->second;
        else
            return Data();
    }

    ValueVector Get_Value_Vector(DataMap &vectors, std::string key)
    {
        std::map<std::string, Data>::iterator search = vectors.find(key);
        if (search != vectors.end())
            return search->second.values;
        else
            return {};
    }

	ValueVector& Get_Value_Vector_Ref(DataMap &vectors, std::string key)
	{
		std::map<std::string, Data>::iterator search = vectors.find(key);
		if (search == vectors.end())
			throw std::domain_error("Vector not found");

		return search->second.values;
	}

    std::string Format_Decimal(double number, int precision)
    {
        std::stringstream str;
        str << std::fixed << std::setprecision(precision) << number;
        return str.str();
    }

    std::string Format_Date(time_t t)
    {
#ifdef _WIN32
        tm tim;
        _localtime(&tim, t);
        tm* timeinfo = &tim;
#else
        tm* timeinfo = nullptr;
        _localtime(timeinfo, t);
#endif
        if (!timeinfo)
            return "???";

        std::ostringstream str;
        int min = timeinfo->tm_min;

        str << timeinfo->tm_mday << "." << timeinfo->tm_mon + 1 << "." << timeinfo->tm_year + 1900 << " " << timeinfo->tm_hour << ":";
        if (min < 10)
            str << "0";

        str << min;
        return str.str();
    }

    std::string Format_Date_CSV(time_t t)
    {
#ifdef _WIN32
        tm tim;
        _localtime(&tim, t);
        tm* timeinfo = &tim;
#else
        tm* timeinfo = nullptr;
        _localtime(timeinfo, t);
#endif
        if (!timeinfo)
            return "???";

        char buffer[80];
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S", timeinfo);

        return std::string(buffer);
    }
}
