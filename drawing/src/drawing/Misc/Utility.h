/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <vector>
#include <ctime>
#include <map>
#include "../Containers/Value.h"

// utility namespace, to better recognize generic methods from the rest
namespace Utility
{
    extern const std::vector<std::string> Curve_Colors;

    // finds index of IST value in vector on date supplied
    int Find_Ist_Index(double date, ValueVector &ist);

    // finds IST value on vector on date supplied
    double Find_Ist_Level(double date, ValueVector &ist);

    // retrieves times range for plot drawing
    std::vector<time_t> Get_Times(time_t minDate, time_t maxDate);

    // retrieves point Y coordinate (value) on quadratic bezier line
    Value Get_Bezier_Value(Value &p0, Value &p1, Value &p2, Value &search);

    // converts time in seconds (unix time) to double
    double Time_To_Double(time_t time);

    // retrieves boundary dates of all values present in both vectors
    void Get_Boundary_Dates(ValueVector &istVector, ValueVector &bloodVector, time_t &min_date, time_t &max_date);

    // retrieves values quartile
    double Get_Quartile(double index, std::vector<double> &vector);

    // finds a value (interpolates if needed) in a given vector due to specified date (measuredBlood); the result is stored to searchValue
    bool Find_Value(Value &searchValue, Value &measuredBlood, ValueVector &diffVal);

    // retrieves data item from data map by given key; returns empty data item if not found
    Data Get_Data_Item(DataMap &vectors, std::string key);

    // retrieves value vector from data map by given key; returns empty vector if not found
    ValueVector Get_Value_Vector(DataMap &vectors, std::string key);

    // converts glucose concentration from mmol/l to mg/dl
    double MmolL_To_MgDl(double mmol);

    // converts glucose concentration from mg/dl to mmol/l
    double MgDl_To_MmolL(double mgdl);

    // formats decimal number to string
    std::string Format_Decimal(double number, int precision);

    // formats input unix timestamp to date string
    std::string Format_Date(time_t t);

    // formats input unix timestamp to date string used for CSV output
    std::string Format_Date_CSV(time_t t);
}
