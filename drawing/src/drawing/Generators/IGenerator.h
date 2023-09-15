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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <map>
#include <vector>

#include "../Containers/Value.h"
#include "../SVGBuilder.h"

class Day;

/*
 * Abstract class / Interface for all plot drawing methods
 */
class IGenerator
{
    private:
        //

    protected:
        // SVG file generator
        SVG mSvg;

        // stored input data
        DataMap &mInputData;
        // localization map common for all methods
        LocalizationMap &mLocalization;

        // maximum value in input data
        double mMaxValue;
        // input units flag - true: mmol/l, false: mg/dl ; TODO: rework this to enum
        bool mMmolFlag;

        // normalize X coordinate using input X value
        virtual double Normalize_X(double x) const { return x; };
        // normalize X coordinate using input hour and minute
        virtual double Normalize_X(int hour, int minute) const { return 0.0; };
        // normalize X coordinate using input time value
        virtual double Normalize_Time_X(time_t x) const { return 0.0; };
        // normalize Y coordinate using input Y value
        virtual double Normalize_Y(double y) const { return y; };

    public:
        IGenerator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
            : mInputData(inputData), mLocalization(localization), mMaxValue(maxValue), mMmolFlag(mmolFlag)
        {
        }

        // interface method - this method builds SVG string of specific plot using input data (constructor) and returns it
        virtual std::string Build_SVG() = 0;

        // find localized string using supplied key; the name is chosen to be the same as the Wt localization function
        std::string tr(std::string key)
        {
            LocalizationMap::iterator search = mLocalization.find(key);
            if (search != mLocalization.end())
                return search->second;
            else
                return "!not loaded!";
        }
};
