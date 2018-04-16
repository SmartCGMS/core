/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include <sstream>
#include "Diff3.h"

Diff3::Diff3()
{
    empty = false;
    visible = true;
}

Diff3::~Diff3()
{
}

std::string Diff3::To_String() const
{
    std::stringstream stream;
    stream  << "param 3" << std::endl
            << "p ,"    << p << std::endl
            << "cg ,"   << cg << std::endl
            << "c ,"    << c << std::endl
            << "dt ,"   << dt << std::endl
            << "h ,"    << h << std::endl
            << "kpos ," << kpos << std::endl
            << "kneg ," << kneg << std::endl
            << "tau ,"  << tau << std::endl;

    return stream.str();
}
