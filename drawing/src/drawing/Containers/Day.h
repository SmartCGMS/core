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
