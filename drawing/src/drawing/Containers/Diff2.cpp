/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include <sstream>
#include "Diff2.h"

Diff2::Diff2()
{
    empty = false;
    visible = true;
}

Diff2::~Diff2()
{
}

std::string Diff2::To_String() const
{
    std::stringstream stream;
    stream  << "param 2" << std::endl
            << "p ,"    << p << std::endl
            << "dt ,"   << dt << std::endl
            << "c ,"    << c << std::endl
            << "s ,"    << s << std::endl
            << "cg ,"   << cg << std::endl
            << "h ,"    << h << std::endl
            << "k ,"    << k << std::endl;

    return stream.str();
}
