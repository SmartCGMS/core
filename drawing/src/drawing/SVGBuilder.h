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

#include <sstream>

/*
 * SVG generator class; may be used as "part of SVG" generator, i.e. for parallel creation of more groups
 */
class SVG
{
    private:
        //

    protected:
        // internal stream
        std::stringstream mSvgStream;

        // current stroke width
        int mWidthStroke;
        // current stroke color
        std::string mColor;
        // current fill color
        std::string mFill;
        // current fill opacity
        double mFillOpacity;
        // current stroke opacity
        double mStrokeOpacity;
        // current stroke visibility
        bool mVisible;

    public:
        SVG();

        // changes stroke visibility state for any further drawing
        void Set_Stroke_Visible(bool visibleP);
        // sets stroke properties for any further drawing
        void Set_Stroke(int width, std::string new_color, std::string new_fill);
        // sets stroke properties for any further drawing
        void Set_Stroke(int width, std::string new_color, std::string new_fill, double fill_opacityP, double stroke_opacityP);
        // resets stroke default properties for any further drawing
        void Set_Default_Stroke();
        // retrieves current stroke properties string
        std::string Get_Stroke() const;

        // draws line using given points
        void Line(double x1, double y1, double x2, double y2);
        // draws dashed line using given points
        void Line_Dash(double x1, double y1, double x2, double y2);
        // draws point (circle) of given radius
        void Point(double x1, double y1, int radius);
        // draws text
        void Draw_Text(double x1, double y1, std::string text, std::string align, std::string color, int font_size);
        // draws transformed text
        void Draw_Text_Transform(double x1, double y1, std::string text, std::string transform, std::string color, int font_size = 0);

        // writes SVG header into stream
        void Header(int sizeX, int sizeY);
        // writes SVG footer into stream
        void Footer();

        // opens group of given ID and appends stroke parameter, if requested; consider using SVG::GroupGuard RAII structure
        void Group_Open(std::string id, bool addStroke);
        // closes single group
        void Group_Close();

        // writes link with onClick event bound
        void Link_Text(double x, double y, std::string text, std::string function, int font_size = 0);
        // writes link of current color with onClick event bound
        void Link_Text_color(double x, double y, std::string text, std::string function, int font_size = 0);

        // draws rectangle of given properties
        void Rectangle(double x, double y, double width, double height);

        // dumps SVG to string and returns it
        std::string Dump() const;

        // RAII guard class for opening and closing groups (<g> tag)
        class GroupGuard
        {
            private:
                SVG& mParent;

            public:
                GroupGuard(SVG& parent, std::string id, bool addStroke);
                ~GroupGuard();
        };

        // this allows external writing into SVG using standard stream operator <<
        template<typename T>
        std::ostream& operator<<(T const& rval)
        {
            return mSvgStream << rval;
        }
};
