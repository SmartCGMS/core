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

#include "../general.h"
#include "DayGenerator.h"

#include "../SVGBuilder.h"
#include "../Containers/Value.h"
#include "../Containers/Day.h"
#include "../Misc/Utility.h"

int CDay_Generator::startX = 50;
int CDay_Generator::maxX = 700;
int CDay_Generator::maxY = 550;
int CDay_Generator::sizeX = 800;
int CDay_Generator::sizeY = 750;

constexpr double Day_Max_Mgdl = 550.0;

void CDay_Generator::Set_Canvas_Size(int width, int height)
{
	sizeX = width;
	sizeY = height;

	maxX = width;
	maxY = height;
}

void CDay_Generator::Write_Description()
{
    mSvg.Set_Default_Stroke();

    // description group scope
    {
        SVG::GroupGuard grp(mSvg, "description", true);

        // y-axis
        std::ostringstream rotate;
        double yPom = (maxY / 2);
        rotate << "rotate(-90 " << startX - 40 << "," << yPom << ")";
        std::stringstream str;
        str << tr("concentration");
        str << (mMmolFlag ? " [mmol/l] " : " [mg/dl] ");
        mSvg.Draw_Text_Transform(startX - 40, yPom, str.str(), rotate.str(), "black", 14);
        mSvg.Line(startX, Normalize_Y(maxY), startX, Normalize_Y(0));

        // x-axis
        mSvg.Draw_Text(sizeX / 2, Normalize_Y(0) + 50, tr("timeDay"), "middle", "black", 14);
        mSvg.Line(startX, Normalize_Y(0), Normalize_X(24, 0), Normalize_Y(0));

        // x-axis description
        for (int i = 0; i <= 24; i += 1)
        {
            double x_d = Normalize_X(i, 0);
            mSvg.Line(x_d, Normalize_Y(0), x_d, Normalize_Y(0) + 10);
            mSvg.Draw_Text(x_d, Normalize_Y(0) + 30, std::to_string(i), "middle", "black", 12);
        }

		const int step = (int)(Day_Max_Mgdl / 15.0);

        // y-axis description
        for (int i = 0; i <= Day_Max_Mgdl; i += step)
        {
            double y_d = Normalize_Y(i);
            mSvg.Line(startX - 10, y_d, startX, y_d);
            mSvg.Draw_Text(startX - 25, y_d, mMmolFlag ? Utility::Format_Decimal(Utility::MgDl_To_MmolL(i), 3) : std::to_string(i), "middle", "black", 12);
        }
    }

    mSvg.Set_Default_Stroke();
}

void CDay_Generator::Write_Legend()
{
    mSvg.Set_Default_Stroke();

    // legend group scope
    {
        SVG::GroupGuard grp(mSvg, "legend", false);

		size_t curColorIdx = 0;
		double y = 40;

		// ist legend group scope
		{
			SVG::GroupGuard istGrp(mSvg, "ist", false);
			mSvg.Set_Stroke(1, "blue", "none");
			mSvg.Link_Text_color(startX + 10, y, tr("ist"), "change_visibility_ist()", 12);

			y += 20;
		}

		for (auto& dataVector : mInputData)
		{
			if (dataVector.second.calculated && !dataVector.second.empty)
			{
				mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], Utility::Curve_Colors[curColorIdx]);

				SVG::GroupGuard diffGrp(mSvg, dataVector.second.identifier, false);
				mSvg.Link_Text_color(startX + 10, y, tr(dataVector.second.identifier), "", 12); // TODO: add visibility change function

				curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();

				y += 20;
			}
		}

		// blood group scope
		if (!Utility::Get_Value_Vector(mInputData, "blood").empty())
		{
			mSvg.Set_Stroke(1, "red", "red");

			SVG::GroupGuard bloodGrp(mSvg, "blood", false);
			mSvg.Link_Text_color(startX + 10, y, tr("blood"), "change_visibility_blood()", 12);

			y += 20;
		}

		// blood calibration group scope
		if (!Utility::Get_Value_Vector(mInputData, "bloodCalibration").empty())
		{
			mSvg.Set_Stroke(1, "#EC80FF", "#EC80FF");

			SVG::GroupGuard bloodGrp(mSvg, "bloodCalibration", false);
			mSvg.Link_Text_color(startX + 10, y, tr("bloodCalibration"), "change_visibility_bloodCalibration()", 12);
		}

        mSvg.Set_Default_Stroke();
    }
}

void CDay_Generator::Write_QuadraticBezireCurve(std::vector<TCoordinate> values)
{
    if (values.empty())
        return;

    size_t i;

    TCoordinate& start = values[0];
    mSvg << "<path d=\"M " << start.x << " " << start.y;

    for (i = 0; i < values.size() - 1; i += 2)
    {
        TCoordinate& control = values[i];
        TCoordinate& end = values[i + 1];

        mSvg << " Q " << control.x << " " << control.y << " " << end.x << " " << end.y;
    }

    mSvg << "\"/>" << std::endl;
}

void CDay_Generator::Write_Normalized_Lines(ValueVector &values, std::string nameGroup, std::string color)
{
    if (values.empty())
        return;

    size_t i;

    mSvg.Set_Default_Stroke();
    mSvg.Set_Stroke(1, color, "none");

    // line set group scope
    {
        SVG::GroupGuard grp(mSvg, nameGroup, true);

        std::vector<TCoordinate> coordinates;

        double last_x = -1, x, y;
        uint64_t last_segment = 0;

        for (i = 0; i < values.size(); i++)
        {
            Value& val = values[i];

            x = Normalize_Time_X(val.date);
            y = Normalize_Y(Utility::MmolL_To_MgDl(val.value));

            if (x < last_x || (last_segment != 0 && val.segment_id != last_segment))
            {
                Write_QuadraticBezireCurve(coordinates);
                coordinates.clear();
            }

			coordinates.push_back(TCoordinate{ x, y });
            last_x = x;
            last_segment = val.segment_id;
        }

        Write_QuadraticBezireCurve(coordinates);
    }
}

void CDay_Generator::Write_Body()
{
    ValueVector istVector = Utility::Get_Value_Vector(mInputData, "ist");
    ValueVector bloodVector = Utility::Get_Value_Vector(mInputData, "blood");
	ValueVector bloodCalibrationVector = Utility::Get_Value_Vector(mInputData, "bloodCalibration");

    Write_Normalized_Lines(istVector, "istCurve", "blue");

	size_t curColorIdx = 0;

	for (auto& dataVector : mInputData)
	{
		if (dataVector.second.calculated && !dataVector.second.empty)
		{
			Write_Normalized_Lines(dataVector.second.values, dataVector.second.identifier + "Curve", Utility::Curve_Colors[curColorIdx]);

			curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();
		}
	}

	// blood curve group scope
	{
		SVG::GroupGuard grp(mSvg, "bloodCurve", false);

		for (size_t i = 0; i < bloodVector.size(); i++)
		{
			Value& val = bloodVector[i];
			mSvg.Set_Stroke(3, "red", "red");
			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(Utility::MmolL_To_MgDl(val.value)), 3);
		}
	}

	// blood calibration curve group scope
	{
		SVG::GroupGuard grp(mSvg, "bloodCalibrationCurve", false);

		for (size_t i = 0; i < bloodCalibrationVector.size(); i++)
		{
			Value& val = bloodCalibrationVector[i];
			mSvg.Set_Stroke(3, "#EC80FF", "#EC80FF");
			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(Utility::MmolL_To_MgDl(val.value)), 3);
		}
	}

    Write_Legend();
    Write_Description();
}

double CDay_Generator::Normalize_X(int hour, int minute) const
{
    const double perHour = (double)(maxX - startX - 20) / 24.0;
    const double perMin = perHour / 60.0;
    return startX + ((double)hour*perHour) + ((double)minute*perMin);
}

double CDay_Generator::Normalize_Time_X(time_t date) const
{
#ifdef _WIN32
    tm tim;
    _localtime(&tim, date);
    tm* timeinfo = &tim;
#else
    tm* timeinfo = nullptr;
    _localtime(timeinfo, date);
#endif
    return Normalize_X(timeinfo->tm_hour, timeinfo->tm_min);
}

double CDay_Generator::Normalize_Y(double val) const
{
    double v = maxY * (val / Day_Max_Mgdl) * 0.85;
    // invert axis
    return (maxY - v) - 0.10*maxY;
}

std::string CDay_Generator::Build_SVG()
{
    mSvg.Header(sizeX, sizeY);
    Write_Body();
    mSvg.Footer();

    return mSvg.Dump();
}

CDay_Generator::CDay_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
    : IGenerator(inputData, maxValue, localization, mmolFlag)
{
    //
}
