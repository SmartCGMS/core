/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include "IGenerator.h"

struct Coordinate;
struct Value;
class Stats;

/*
 * Graph (overall) generator class
 */
class CGraph_Generator : public IGenerator
{
    private:
        //

    protected:
        // maximum value on Y axis
        double mMaxValueY;
        // time limits
        time_t mMinD, mMaxD, mDifD;

        // writes plot body
        void Write_Body();
        // writes plot description (axis titles, ..)
        void Write_Description();
        // writes normalized set of lines
        void Write_Normalized_Lines(ValueVector istVector, ValueVector bloodVector);
        // writes bezier curve and counts errors on the path from supplied value vector values
        void Write_QuadraticBezierCurve(ValueVector values);

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
        CGraph_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag);

        static void Set_Canvas_Size(int width, int height);

        virtual std::string Build_SVG() override;
};
