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

/*
 * Day statistics generator class
 */
class CDay_Generator : public IGenerator
{
    private:
        //

    protected:
        // writes main plot body
        void Write_Body();
        // writes set of normalized lines into group
        void Write_Normalized_Lines(ValueVector &values, std::string nameGroup, std::string color);
        // writes quadratic beziere curve
        void Write_QuadraticBezireCurve(std::vector<Coordinate> values);
        // writes plot legent
        void Write_Legend();
        // writes plot description
        void Write_Description();

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
        CDay_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag);

        static void Set_Canvas_Size(int width, int height);

        virtual std::string Build_SVG() override;
};
