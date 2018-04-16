/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include "../Generators/IGenerator.h"

struct Coordinate;

/*
 * Clark generator class
 */
class CClark_Generator : public IGenerator
{
    private:
        //

    protected:
        // writes main plot body
        void Write_Body();
        // writes one item of legend
        void Write_Legend_Item(std::string id, std::string text, std::string function, double y);
        // writes plot description (axes titles, ..)
        void Write_Descripton();
        // writes plot background map
        void Write_Plot_Map();
        // writes path normalized using local normalize methods
        void Write_Normalized_Path(std::vector<Coordinate> const& values);

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
        CClark_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag);

        static void Set_Canvas_Size(int width, int height);

        virtual std::string Build_SVG() override;
};
