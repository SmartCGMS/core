/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "Value.h"
#include "../Misc/Utility.h"
#include <cmath>
#include <algorithm>

Value::Value(double _value, time_t _date, uint64_t _segment_id) : value(_value), date(_date), segment_id(_segment_id)
{
}

Value::~Value()
{
}

double Value::Get_Value()
{
    return value;
}

void Value::Set_Value(double _value)
{
    value = _value;
}

Stats::Stats()
{
    mSumAbsolute = 0;
    mSumRelative = 0;
    mIsSorted = false;
    mPrintStats = false;
}

Stats::~Stats()
{
}

void Stats::Sort()
{
    std::sort(mAbsuluteError.begin(), mAbsuluteError.end());
    std::sort(mRelativeError.begin(), mRelativeError.end());
    mIsSorted = true;
}

void Stats::Quartile_Rel(double& q0, double& q1, double& q2, double& q3, double& q4)
{
    if (mRelativeError.empty())
        return;

    if (!mIsSorted)
        Sort();

    q0 = *(mRelativeError.begin());
    double g1 = mRelativeError.size() * 0.25;
    q1 = Utility::Get_Quartile(g1, mRelativeError);
    q2 = Utility::Get_Quartile(mRelativeError.size() * 0.5, mRelativeError);
    double g3 = mRelativeError.size() * 0.75;
    q3 = Utility::Get_Quartile(g3, mRelativeError);
    q4 = *(mRelativeError.end() - 1);
}

double Stats::Avarage_Rel() const
{
    return mSumRelative / (double)mRelativeError.size();
}

void Stats::Quartile_Abs(double& q0, double& q1,double& q2, double& q3, double& q4)
{
    if (mAbsuluteError.empty())
        return;

    if (!mIsSorted)
        Sort();

    q0 = *(mAbsuluteError.begin());
    double g1 = mAbsuluteError.size() * 0.25;
    q1 = Utility::Get_Quartile(g1, mAbsuluteError);
    q2 = Utility::Get_Quartile(mAbsuluteError.size() * 0.5, mAbsuluteError);
    double g3 = mAbsuluteError.size() * 0.75;
    q3 = Utility::Get_Quartile(g3, mAbsuluteError);
    q4 = *(mAbsuluteError.end() - 1);
}

double Stats::Avarage_Abs() const
{
    return mSumAbsolute / (double)mAbsuluteError.size();
}

void Stats::Next_Error(double measured, double counted)
{
    mIsSorted = false;

    double absolute = fabs(measured - counted);
    double relative = absolute / measured;

    mSumAbsolute += absolute;
    mSumAbsolute += relative;

    mAbsuluteError.push_back(absolute);
    mRelativeError.push_back(relative);
}

void Stats::Set_Print_Stats(bool state)
{
    mPrintStats = state;
}

bool Stats::Get_Print_Stats() const
{
    return mPrintStats;
}

Data::Data()
{
    visible = false;
    empty = true;
    calculated = false;
}

Data::~Data()
{
    values.clear();
}

Data::Data(ValueVector _values, bool _visible, bool _empty, bool _calculated) : values(_values), visible(_visible), empty(_empty), calculated(_calculated)
{
}

bool Data::Is_Visible()
{
    return visible && !empty;
}

bool Data::Is_Visible_Legend()
{
    return !empty;
}
