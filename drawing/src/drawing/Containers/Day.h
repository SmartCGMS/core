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
        bool mEmpty = true;

    protected:
        void Quartile_X(std::vector<double> vector, double &y0, double &y1, double &y2, double &y3, double &y4);

    public:
        Day();
        ~Day();
        void Insert(double key, std::vector<double> value);
        bool Contains(double key) const;
        void Quartile(std::vector<TCoordinate> &q0, std::vector<TCoordinate> &q1, std::vector<TCoordinate> &q2, std::vector<TCoordinate> &q3, std::vector<TCoordinate> &q4);
        bool Is_Empty() const;
        void Set_Empty(bool state);
};
