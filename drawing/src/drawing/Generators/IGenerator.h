/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
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
