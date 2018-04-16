/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <string>

struct Diff2
{
    Diff2();
    ~Diff2();

    std::string To_String() const;

    double p;
    double dt;
    double c;
    double s;
    double cg;
    double h;
    double k;

    bool empty;
    bool visible;
};
