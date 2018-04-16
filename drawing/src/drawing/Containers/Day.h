/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <map>
#include <vector>
#include "Value.h"

class Day
{
    private:
        std::map<double, std::vector<double> > mMap;
        bool mEmpty;

    protected:
        void Quartile_X(std::vector<double> vector, double &y0, double &y1, double &y2, double &y3, double &y4);

    public:
        Day();
        ~Day();
        void Insert(double key, std::vector<double> value);
        bool Contains(double key) const;
        void Quartile(std::vector<Coordinate> &q0, std::vector<Coordinate> &q1, std::vector<Coordinate> &q2, std::vector<Coordinate> &q3, std::vector<Coordinate> &q4);
        bool Is_Empty() const;
        void Set_Empty(bool state);
};
