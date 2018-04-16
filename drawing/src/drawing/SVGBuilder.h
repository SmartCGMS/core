/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
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
