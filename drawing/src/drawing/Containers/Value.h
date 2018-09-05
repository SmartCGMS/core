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

#include <ctime>
#include <map>
#include <vector>
#include <string>

typedef std::map<std::string, std::string> LocalizationMap;

struct Value
{
    Value(double _value = 0.0, time_t _date = 0, uint64_t _segment_id = 0);
    ~Value();

    double Get_Value();
    void Set_Value(double _value);

    double value;
    time_t date;
    time_t normalize;
    uint64_t segment_id;
};

typedef std::vector<Value> ValueVector;

struct TCoordinate {
    double x, y;
};

class Stats
{
    private:
        std::vector<double> mAbsuluteError;
        std::vector<double> mRelativeError;
        double mSumAbsolute;
        double mSumRelative;
        bool mIsSorted;
        bool mPrintStats;

    public:
        Stats();
        ~Stats();

        void Sort();
        void Quartile_Rel(double& q0, double& q1, double& q2, double& q3, double& q4);
        double Avarage_Rel() const;
        void Quartile_Abs(double& q0, double& q1, double& q2, double& q3, double& q4);
        double Avarage_Abs() const;
        void Next_Error(double measured, double counted);

        void Set_Print_Stats(bool state);
        bool Get_Print_Stats() const;
};

struct Data
{
    Data();
    ~Data();
    Data(ValueVector _values, bool _visible, bool _empty, bool _calculated = false);

    bool Is_Visible();
    bool Is_Visible_Legend();

    ValueVector values;
    bool visible = false;
    bool empty = true;
	bool calculated = false;
	std::string identifier;
	std::string refSignalIdentifier;
};

typedef std::map<std::string, Data> DataMap;
