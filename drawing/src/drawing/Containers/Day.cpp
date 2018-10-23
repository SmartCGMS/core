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
 * Univerzitni 8
 * 301 00, Pilsen
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
 *       monitoring", Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 */

#include <algorithm>
#include <ctime>

#include "Day.h"
#include "../Misc/Utility.h"

static bool compare_function(TCoordinate c1, TCoordinate c2) 
{
    return (c1.x < c2.x); 
}

Day::Day()
{
    mEmpty = false;
}

Day::~Day()
{
}

void Day::Insert(double key, std::vector<double> value)
{
    std::sort(value.begin(), value.end());

    mMap[key] = value;
}

bool Day::Contains(double key) const
{
    return (mMap.find(key) != mMap.end());
}

void Day::Quartile(std::vector<TCoordinate> &q0, std::vector<TCoordinate> &q1, std::vector<TCoordinate> &q2, std::vector<TCoordinate> &q3, std::vector<TCoordinate> &q4)
{
    if (!mMap.empty())
    {
        for (auto it = mMap.begin(); it != mMap.end(); ++it)
        {
            double key = it->first;
            std::vector<double> vector = it->second;
            double y0 = 0.0, y1 = 0.0, y2 = 0.0, y3 = 0.0, y4 = 0.0;

            Quartile_X(vector, y0, y1, y2, y3, y4);
			q0.push_back(TCoordinate{ key, y0 });
			q1.push_back(TCoordinate{ key, y1 });
			q2.push_back(TCoordinate{ key, y2 });
			q3.push_back(TCoordinate{ key, y3 });
			q4.push_back(TCoordinate{ key, y4 });
        }
    }

    std::sort(q0.begin(), q0.end(), compare_function);
    std::sort(q1.begin(), q1.end(), compare_function);
    std::sort(q2.begin(), q2.end(), compare_function);
    std::sort(q3.begin(), q3.end(), compare_function);
    std::sort(q4.begin(), q4.end(), compare_function);
}

void Day::Quartile_X(std::vector<double> vector, double &y0, double &y1, double &y2, double &y3, double &y4)
{
    if (vector.empty())
        return;

    double size = (double)vector.size();

    y0 = *(vector.begin());
    y1 = Utility::Get_Quartile(size * 0.25, vector);
    y2 = Utility::Get_Quartile(size * 0.5, vector);
    y3 = Utility::Get_Quartile(vector.size() * 0.75, vector);
    y4 = *(vector.end() - 1);
}

bool Day::Is_Empty() const
{
    return mEmpty;
}

void Day::Set_Empty(bool state)
{
    mEmpty = state;
}
