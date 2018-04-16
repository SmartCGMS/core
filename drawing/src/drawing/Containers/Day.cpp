/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include <algorithm>
#include <ctime>

#include "Day.h"
#include "../Misc/Utility.h"

static bool compare_function(Coordinate c1, Coordinate c2) 
{
    return (c1.x < c2.x); 
}

Day::Day()
{
    mEmpty = false;
}

Day::~Day()
{
}

void Day::Insert(double key, std::vector<double> value)
{
    std::sort(value.begin(), value.end());

    mMap[key] = value;
}

bool Day::Contains(double key) const
{
    return (mMap.find(key) != mMap.end());
}

void Day::Quartile(std::vector<Coordinate> &q0, std::vector<Coordinate> &q1, std::vector<Coordinate> &q2, std::vector<Coordinate> &q3, std::vector<Coordinate> &q4)
{
    if (!mMap.empty())
    {
        for (auto it = mMap.begin(); it != mMap.end(); ++it)
        {
            double key = it->first;
            std::vector<double> vector = it->second;
            double y0, y1, y2, y3, y4;

            Quartile_X(vector, y0, y1, y2, y3, y4);
            q0.push_back(Coordinate(key, y0));
            q1.push_back(Coordinate(key, y1));
            q2.push_back(Coordinate(key, y2));
            q3.push_back(Coordinate(key, y3));
            q4.push_back(Coordinate(key, y4));
        }
    }

    std::sort(q0.begin(), q0.end(), compare_function);
    std::sort(q1.begin(), q1.end(), compare_function);
    std::sort(q2.begin(), q2.end(), compare_function);
    std::sort(q3.begin(), q3.end(), compare_function);
    std::sort(q4.begin(), q4.end(), compare_function);
}

void Day::Quartile_X(std::vector<double> vector, double &y0, double &y1, double &y2, double &y3, double &y4)
{
    if (vector.empty())
        return;

    double size = (double)vector.size();

    y0 = *(vector.begin());
    y1 = Utility::Get_Quartile(size * 0.25, vector);
    y2 = Utility::Get_Quartile(size * 0.5, vector);
    y3 = Utility::Get_Quartile(vector.size() * 0.75, vector);
    y4 = *(vector.end() - 1);
}

bool Day::Is_Empty() const
{
    return mEmpty;
}

void Day::Set_Empty(bool state)
{
    mEmpty = state;
}
