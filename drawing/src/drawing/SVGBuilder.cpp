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

SVG::SVG() : mFillOpacity(-1), mStrokeOpacity(-1), mVisible(true)
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
