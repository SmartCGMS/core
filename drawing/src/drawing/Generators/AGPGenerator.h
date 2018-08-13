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
