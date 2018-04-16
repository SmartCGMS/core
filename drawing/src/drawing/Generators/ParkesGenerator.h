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

/*
 * Parkes generator class
 */
class CParkes_Generator : public IGenerator
{
    private:
        //

    protected:
        // is diabetes type 1? true = type 1, false = type 2
        bool mDiabetesType1;

        // writes main plot body
        void Write_Body();
        // writes plot description (axis titles, ...)
        void Write_Description();
        // writes single legend item
        void Write_Legend_Item(std::string id, std::string text, std::string function, double y);
        // writes background map for diabetes type 1
        void Write_Background_Map_Type1();
        // writes background map for diabetes type 2
        void Write_Background_Map_Type2();
        // writes normalized set of lines
        void Write_Normalized_Lines(std::vector<Coordinate> values);

        virtual double Normalize_X(double x) const override;
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
        CParkes_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag, bool diabetesType1);

        static void Set_Canvas_Size(int width, int height);

        virtual std::string Build_SVG() override;
};
