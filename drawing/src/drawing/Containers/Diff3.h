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

struct Diff3
{
    Diff3();
    ~Diff3();

    std::string To_String() const;

    double p;
    double cg;
    double c;
    double dt;
    double h;
    double kpos;
    double kneg;
    double tau;

    bool empty;
    bool visible;
};
