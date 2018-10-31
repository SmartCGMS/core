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
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
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
