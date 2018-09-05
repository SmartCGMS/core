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

#include "IGenerator.h"

struct TCoordinate;

/*
 * AGP generator class
 */
class CAGP_Generator : public IGenerator
{
    private:
        //

    protected:
        // aggregates all X values present in all data sets
        Day Aggregate_X_Axis(Data istData, Data bloodData);

        // writes plot body
        void Write_Body();
        // writes bottom legend
        void Write_Legend(Data istData, Data bloodData, Data bloodCalibrationData);
        // writes main plot content - data and everything related
        void Write_Content(Day day, Data bloodData, Data bloodCalibrationData);
        // writes plot description (axis titles, ..)
        void Write_Description();

        // draws bezier curve
        void Write_QuadraticBezirCurve_1(std::vector<TCoordinate> &values, std::vector<TCoordinate> &back);
        // draws bezier curve
        void Write_QuadraticBezirCurve_2(std::vector<TCoordinate> &values);

        virtual double Normalize_X(int hour, int minute) const override;
        virtual double Normalize_Time_X(time_t x) const override;
        virtual double Normalize_Y(double y) const override;

        // image start X coordinate
        static int startX;
        // image maximum X coordinate (excl. padding)
        static int maxX;
        // image maximum Y coordinate (excl. padding)
        static int maxY;
        // image limit X coordinate
        static int sizeX;
        // image limit Y coordinate
        static int sizeY;

    public:
        CAGP_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag);

        static void Set_Canvas_Size(int width, int height);

        virtual std::string Build_SVG() override;
};
