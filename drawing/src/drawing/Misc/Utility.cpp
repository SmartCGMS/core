/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include "../general.h"
#include "Utility.h"

#include <iomanip>

namespace Utility
{
    const std::array<const char*, 5> Curve_Colors = { "orange", "green", "cyan", "magenta", "purple" };

    double Time_To_Double(time_t time)
    {
        return difftime(time, 0);
    }

    int Find_Ist_Index(double date, ValueVector &ist)
    {
        for (size_t i = 0; i < ist.size() - 1; i++)
        {
            Value& v = ist[i];
            Value& next = ist[i + 1];
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
        double t = Time_To_Double((search.date - p0.date) / (search.date - p0.date));
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
        int in = (int)intPart;

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

        int indexShift = 0;

        Value& countedDiff = diffVal[index];
        // exact match - no need to interpolate
        if (measuredBlood.date == countedDiff.date)
            searchValue = countedDiff;
        else // otherwise approximate
        {
            // border value - no data
            if (index == 0 || index == diffVal.size())
                return false;

            // even index - bezier curve passes through this point
            if (index % 2 != 0)
                indexShift = 1;

            Value &p2 = diffVal[index + indexShift];
            Value &p1 = diffVal[index + indexShift - 1];
            Value &p0 = diffVal[index + indexShift - 2];

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

    std::string Format_Decimal(double number, int precision)
    {
        std::stringstream str;
        str << std::setprecision(precision) << number;
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
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S", timeinfo);

        return std::string(buffer);
    }
}
