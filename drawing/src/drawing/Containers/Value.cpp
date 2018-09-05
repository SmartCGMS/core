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
