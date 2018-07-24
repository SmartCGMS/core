/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include "../general.h"
#include "ParkesGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/PlotBackgroundMap.h"
#include "../Misc/Utility.h"

int CParkes_Generator::startX = 100;
int CParkes_Generator::maxX = 550;
int CParkes_Generator::maxY = 550;
int CParkes_Generator::sizeX = 750;
int CParkes_Generator::sizeY = 750;

constexpr double Parkes_Norm_Limit = 630.0;
constexpr double Parkes_Max_MgDl = 550.0;

void CParkes_Generator::Set_Canvas_Size(int width, int height)
{
	// we need the image to be square
	if (width < height)
		height = width;
	else
		width = height;

	sizeX = width;
	sizeY = height;

	maxX = width;
	maxY = height;
}

void CParkes_Generator::Write_Background_Map_Type1()
{
    mSvg.Set_Stroke(1, "grey", "none");
    mSvg.Set_Stroke_Visible(false);

    // background map group scope
    {
        SVG::GroupGuard grp(mSvg, "graphMap", true);

        // zona a
        Write_Normalized_Lines(Coords_a_t1);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(500), "A", "middle", "grey", 14);
        // zona b up
        Write_Normalized_Lines(Coords_b_up_t1);
        mSvg.Draw_Text(Normalize_X(300), Normalize_Y(500), "B", "middle", "grey", 14);
        // zona b down
        Write_Normalized_Lines(Coords_b_down_t1);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(320), "B", "middle", "grey", 14);
        // zona c up
        Write_Normalized_Lines(Coords_c_up_t1);
        mSvg.Draw_Text(Normalize_X(175), Normalize_Y(500), "C", "middle", "grey", 14);
        // zona c down
        Write_Normalized_Lines(Coords_c_down_t1);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(165), "C", "middle", "grey", 14);
        // zona d up
        Write_Normalized_Lines(Coords_d_up_t1);
        mSvg.Draw_Text(Normalize_X(70), Normalize_Y(500), "D", "middle", "grey", 14);
        // zona d down
        Write_Normalized_Lines(Coords_d_down_t1);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(50), "D", "middle", "grey", 14);
        // zona e up
        Write_Normalized_Lines(Coords_e_up_t1);
        mSvg.Draw_Text(Normalize_X(10), Normalize_Y(500), "E", "middle", "grey", 14);
    }

    mSvg.Set_Default_Stroke();
}

void CParkes_Generator::Write_Background_Map_Type2()
{
    mSvg.Set_Stroke(1, "grey", "none");

    // background map group scope
    {
        SVG::GroupGuard grp(mSvg, "graphMap", true);
        // zona a
        Write_Normalized_Lines(Coords_a_t2);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(500), "A", "middle", "grey", 12);
        // zona b up
        Write_Normalized_Lines(Coords_b_up_t2);
        mSvg.Draw_Text(Normalize_X(300), Normalize_Y(500), "B", "middle", "grey", 12);
        // zona b down
        Write_Normalized_Lines(Coords_b_down_t2);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(320), "B", "middle", "grey", 12);
        // zona c up
        Write_Normalized_Lines(Coords_c_up_t2);
        mSvg.Draw_Text(Normalize_X(175), Normalize_Y(500), "C", "middle", "grey", 12);
        // zona c down
        Write_Normalized_Lines(Coords_c_down_t2);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(165), "C", "middle", "grey", 12);
        // zona d up
        Write_Normalized_Lines(Coords_d_up_t2);
        mSvg.Draw_Text(Normalize_X(70), Normalize_Y(500), "D", "middle", "grey", 12);
        // zona d down
        Write_Normalized_Lines(Coords_d_down_t2);
        mSvg.Draw_Text(Normalize_X(500), Normalize_Y(50), "D", "middle", "grey", 12);
        // zona e up
        Write_Normalized_Lines(Coords_e_up_t2);
        mSvg.Draw_Text(Normalize_X(10), Normalize_Y(500), "E", "middle", "grey", 12);
    }

    mSvg.Set_Default_Stroke();
}

void CParkes_Generator::Write_Legend_Item(std::string id, std::string text, std::string function, double y)
{
    SVG::GroupGuard grp(mSvg, id, false);
    mSvg.Link_Text_color(startX, y, text, function, 12);
}

void CParkes_Generator::Write_Description()
{
    mSvg.Set_Default_Stroke();
    
    // description group scope
    {
        SVG::GroupGuard grp(mSvg, "description", true);

        std::ostringstream rotate;

        // y-axis
        double yPom = maxY / 2;
        rotate << "rotate(-90 " << startX - 45 << "," << yPom << ")";
        std::stringstream str;
        str << tr("counted");
        str << (mMmolFlag ? " [mmol/l] " : " [mg/dl] ");
        mSvg.Draw_Text_Transform(startX - 45, yPom, str.str(), rotate.str(), "black", 14);
        mSvg.Line(startX, Normalize_Y(Parkes_Max_MgDl), startX, Normalize_Y(0));

        // x-axis
        str.str("");
        str << tr("measured");
        str << (mMmolFlag ? " [mmol/l] " : " [mg/dl] ");
        mSvg.Draw_Text(sizeX / 2, Normalize_Y(0) + 50, str.str(), "middle", "black", 14);
        mSvg.Line(startX, Normalize_Y(0), Normalize_X(Parkes_Max_MgDl), Normalize_Y(0));

		const int step = (int)(Utility::MmolL_To_MgDl(2.0));

        // x-axis description
        for (int i = 0; i <= Parkes_Max_MgDl; i += step)
        {
			double x_d = Normalize_X(i);
            mSvg.Line(x_d, Normalize_Y(0), x_d, Normalize_Y(0) + 10);
            mSvg.Draw_Text(x_d, Normalize_Y(0) + 30, mMmolFlag ? Utility::Format_Decimal(round(Utility::MgDl_To_MmolL(i)), 3) : Utility::Format_Decimal(i, 3), "middle", "black", 12);
        }

        // y-axis description
        for (int i = 0; i <= Parkes_Max_MgDl; i += step)
        {
            double y_d = Normalize_Y(i);
            mSvg.Line(startX - 10, y_d, startX, y_d);
            mSvg.Draw_Text(startX - 25, y_d, mMmolFlag ? Utility::Format_Decimal(round(Utility::MgDl_To_MmolL(i)), 3) : Utility::Format_Decimal(i, 3), "middle", "black", 12);
        }
    }

    mSvg.Set_Stroke(1, "red", "none");
    mSvg.Line_Dash(Normalize_X(0), Normalize_Y(0), Normalize_X(Parkes_Max_MgDl), Normalize_Y(Parkes_Max_MgDl));
    mSvg.Set_Default_Stroke();
}

void CParkes_Generator::Write_Body()
{
    ValueVector istVector = Utility::Get_Value_Vector(mInputData, "ist");
    ValueVector bloodVector = Utility::Get_Value_Vector(mInputData, "blood");

    Write_Description();
    SVG istGr;
	std::map<std::string, SVG> mapGr;

	mSvg.Set_Stroke_Visible(false);
	istGr.Set_Stroke_Visible(false);

	size_t curColorIdx = 0;

	for (auto& dataVector : mInputData)
	{
		if (dataVector.second.calculated && !dataVector.second.empty)
		{
			mapGr[dataVector.second.identifier].Set_Stroke(3, Utility::Curve_Colors[curColorIdx], Utility::Curve_Colors[curColorIdx]);
			mapGr[dataVector.second.identifier].Set_Stroke_Visible(false);
			mapGr[dataVector.second.identifier].Group_Open(dataVector.second.identifier + "Curve", true);

			curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();
		}
	}

    istGr.Set_Stroke(3, "blue", "blue");
    istGr.Group_Open("istCurve", true);

    for (size_t i = 0; i < bloodVector.size(); i++)
    {
        Value& measuredBlood = bloodVector[i];
        Value searchValue;

        if (Utility::Find_Value(searchValue, measuredBlood, istVector))
            istGr.Point(Normalize_X(Parkes_Norm_Limit*Utility::MmolL_To_MgDl(measuredBlood.value) / Parkes_Max_MgDl), Normalize_Y(Parkes_Norm_Limit*Utility::MmolL_To_MgDl(searchValue.value) / Parkes_Max_MgDl), 3);

		for (auto& calcElPair : mapGr)
		{
			if (Utility::Find_Value(searchValue, measuredBlood, mInputData[calcElPair.first].values))
				calcElPair.second.Point(Normalize_X(Parkes_Norm_Limit*Utility::MmolL_To_MgDl(measuredBlood.value) / Parkes_Max_MgDl), Normalize_Y(Parkes_Norm_Limit*Utility::MmolL_To_MgDl(searchValue.value) / Parkes_Max_MgDl), 3);
		}
    }

	for (auto& calcElPair : mapGr)
		calcElPair.second.Group_Close();

    istGr.Group_Close();

	mSvg << istGr.Dump();
	for (auto& calcElPair : mapGr)
		mSvg << calcElPair.second.Dump();

    // legend group scope
    {
        SVG::GroupGuard grp(mSvg, "legend", false);

        double y = Normalize_Y(0) + 70;
        mSvg.Set_Stroke(1, "blue", "blue");
        Write_Legend_Item("ist", tr("counted"), "change_visibility_ist()", y);

		y += 20;

		curColorIdx = 0;

		for (auto& dataVector : mInputData)
		{
			if (dataVector.second.calculated && !dataVector.second.empty)
			{
				mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], Utility::Curve_Colors[curColorIdx]);
				Write_Legend_Item(dataVector.second.identifier, tr(dataVector.second.identifier), "", y); // TODO: proper "change visibility" function

				curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();

				y += 20;
			}
		}

        mSvg.Set_Default_Stroke();
    }
}

double CParkes_Generator::Normalize_X(double date) const
{
	return startX + (date / Parkes_Norm_Limit)*maxX;
}

double CParkes_Generator::Normalize_Y(double val) const
{
	double v = (maxY * val * 0.95) / Parkes_Norm_Limit;
	// invert axis
	return maxY - v - 0.15*sizeY;
}

std::string CParkes_Generator::Build_SVG()
{
    mSvg.Header(sizeX, sizeY);

    if (mDiabetesType1)
        Write_Background_Map_Type1();
    else
        Write_Background_Map_Type2();

    Write_Body();
    mSvg.Footer();

    return mSvg.Dump();
}

CParkes_Generator::CParkes_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag, bool diabetesType1)
    : IGenerator(inputData, maxValue, localization, mmolFlag), mDiabetesType1(diabetesType1)
{
    //
}
