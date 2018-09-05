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
