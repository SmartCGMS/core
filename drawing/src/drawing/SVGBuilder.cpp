/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include <sstream>
#include "SVGBuilder.h"

void SVG::Set_Stroke_Visible(bool visibleP)
{
    mVisible = visibleP;
}

void SVG::Set_Stroke(int width, std::string new_color, std::string new_fill)
{
    mWidthStroke = width;
    mColor = new_color;
    mFill = new_fill;
}

void SVG::Set_Stroke(int width, std::string new_color, std::string new_fill,double fill_opacityP,double stroke_opacityP)
{
    mWidthStroke = width;
    mColor = new_color;
    mFill = new_fill;
    mFillOpacity = fill_opacityP;
    mStrokeOpacity = stroke_opacityP;
}

void SVG::Set_Default_Stroke()
{
    mWidthStroke = 1;
    mColor = "black";
    mFill = "none";
    mFillOpacity = -1;
    mStrokeOpacity = -1;
    mVisible = true;
}

std::string SVG::Get_Stroke() const
{
    if (!mVisible)
        return "";

    std::stringstream oss;
    oss << " stroke-width=\"" << mWidthStroke << "\" stroke=\"" << mColor << "\" stroke-linejoin=\"round\"";
    if (!mFill.empty())
        oss << " fill = \"" << mFill << "\"";

    if (mFillOpacity != -1)
        oss << " fill-opacity=\"" << mFillOpacity << "\"";

    if (mStrokeOpacity != -1)
        oss << " stroke-opacity=\"" << mStrokeOpacity << "\"";

    return oss.str();
}

void SVG::Line(double x1, double y1, double x2, double y2)
{
    mSvgStream << "<line x1 =\"" << x1 << "\" y1 =\"" << y1
               << "\" x2 = \"" << x2 << "\" y2 = \"" << y2
               << "\"" << Get_Stroke()
               << "/>" << std::endl;
}

void SVG::Line_Dash(double x1, double y1, double x2, double y2)
{
    mSvgStream << "<line x1 =\"" << x1 << "\" y1 =\"" << y1
               << "\" x2 = \"" << x2 << "\" y2 = \"" << y2
               << "\"" << Get_Stroke()
               << " stroke-dasharray = \"5,5\""
               << "/>" << std::endl;
}

void SVG::Point(double x1, double y1, int round)
{
    mSvgStream << "<circle cx=\"" << x1 << "\" cy=\"" << y1 << "\" r=\"" << round << "\""
               << Get_Stroke() << " />" << std::endl;
}

void SVG::Draw_Text(double x1, double y1, std::string text, std::string align, std::string color, int font_size)
{
    mSvgStream << "<text x=\"" << x1 << "\" y=\"" << y1
               << "\" text-anchor=\"" << align;

    if (!color.empty())
        mSvgStream << "\" fill=\"" << color;

    mSvgStream << "\" font-size = \"" << font_size << "\">" << text << "</text>" << std::endl;
}

void SVG::Draw_Text_Transform(double x1, double y1, std::string text, std::string transform, std::string color, int font_size)
{
	mSvgStream << "<text x=\"" << x1 << "\" y=\"" << y1
		<< "\" text-anchor=\"middle\" fill=\"" << color << "\" transform=\"" << transform << "\"";
	
	if (font_size)
		mSvgStream << " font-size=\"" << font_size << "\"";

	mSvgStream << ">" << text << "</text>" << std::endl;
}

void SVG::Header(int sizeX, int sizeY)
{
    Set_Default_Stroke();
    mSvgStream << "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"0 0 "
               << sizeX << " " << sizeY << "\" id = \"svgContainer\" " << " shape-rendering=\"geometricPrecision\">" << std::endl;
    mSvgStream << "<g>" << std::endl;
}

void SVG::Footer()
{
    mSvgStream << "</g>" << std::endl;
    mSvgStream << "</svg>" << std::endl;
}

void SVG::Group_Open(std::string id, bool addStroke)
{
    mSvgStream << "<g id = \"" << id << "\" class = \"" << id << "\" ";

    if (addStroke)
    {
        bool pom_visible = mVisible;
        mVisible = true;
        mSvgStream << Get_Stroke();
        mVisible = pom_visible;
    }

    mSvgStream << ">" << std::endl;
}

void SVG::Group_Close()
{
    mSvgStream << "</g>" << std::endl;
}

void SVG::Link_Text(double x, double y, std::string text, std::string function, int font_size)
{
    mSvgStream << "<a class=\"svgRef\" onclick=\"" << function << ";\">";
    Draw_Text(x, y, text, "start", "", font_size ? font_size : 12);
    mSvgStream << "</a>" << std::endl;
}

void SVG::Link_Text_color(double x, double y, std::string text, std::string function, int font_size)
{
    mSvgStream << "<a class=\"svgRef\" fill=\"" << mColor << "\" onclick=\"" << function << ";\">";
    Draw_Text(x, y, text, "start", mColor, font_size ? font_size : 12);
    mSvgStream << "</a>" << std::endl;
}

void SVG::Rectangle(double x, double y, double width, double height)
{
    mSvgStream << "<rect x = \"" << x << "\" y = \"" << y << "\" width = \"" << width << "\" height = \"" << height << "\"/>" << std::endl;
}

std::string SVG::Dump() const
{
    return mSvgStream.str();
}

SVG::SVG() : mVisible(true), mStrokeOpacity(-1), mFillOpacity(-1)
{
}

SVG::GroupGuard::GroupGuard(SVG& parent, std::string id, bool addStroke) : mParent(parent)
{
    mParent.Group_Open(id, addStroke);
}

SVG::GroupGuard::~GroupGuard()
{
    mParent.Group_Close();
}
