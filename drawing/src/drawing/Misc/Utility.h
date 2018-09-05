/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#pragma once

#include <vector>
#include <array>
#include <ctime>
#include <map>
#include "../Containers/Value.h"

// utility namespace, to better recognize generic methods from the rest
namespace Utility
{
    extern const std::array<const char*, 5> Curve_Colors;

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

	// retrieves value vector by reference
	ValueVector& Get_Value_Vector_Ref(DataMap &vectors, std::string key);

	constexpr double Mmoll_To_Mgdl_Const = 18.018;
	constexpr double MgDl_To_Mmoll_Const = 1.0 / Mmoll_To_Mgdl_Const;

    // converts glucose concentration from mg/dl to mmol/l
	constexpr double MgDl_To_MmolL(double mgdl)
	{
		return mgdl * MgDl_To_Mmoll_Const;
	}

	// converts glucose concentration from mmol/l to mg/dl
	constexpr double MmolL_To_MgDl(double mmol)
	{
		return mmol * Mmoll_To_Mgdl_Const;
	}

    // formats decimal number to string
    std::string Format_Decimal(double number, int precision);

    // formats input unix timestamp to date string
    std::string Format_Date(time_t t);

    // formats input unix timestamp to date string used for CSV output
    std::string Format_Date_CSV(time_t t);
}
