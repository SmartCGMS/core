/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include "../general.h"
#include "GraphGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/Utility.h"

#include <algorithm>
#include <set>

constexpr size_t Invalid_Position = (size_t)-1;

int CGraph_Generator::startX = 50;
int CGraph_Generator::maxX = 800;
int CGraph_Generator::maxY = 800;
int CGraph_Generator::sizeX = 800;
int CGraph_Generator::sizeY = 800;

void CGraph_Generator::Set_Canvas_Size(int width, int height)
{
    sizeX = width;
    sizeY = height;

    maxX = width;
    maxY = height;
}

void CGraph_Generator::Write_QuadraticBezierCurve(ValueVector values)
{
	if (values.empty())
		return;

	auto findNext = [&values](size_t &pos, uint64_t segId) -> Value& {
		// the unsigned overflow is well defined, so if "pos" equals Invalid_Position (-1), it will overflow to 0
		for (pos++; pos < values.size(); pos++)
		{
			if (values[pos].segment_id == segId)
				return values[pos];
		}

		pos = Invalid_Position;
		return values[0];
	};

	// extract segment IDs from values
	std::set<uint64_t> segmentIds;
	for (auto& val : values)
		segmentIds.insert(val.segment_id);

	// draw segment lines separatelly
	for (uint64_t segId : segmentIds)
	{
		// use "sliding" approach to avoid multiple iteration and returning back

		size_t pos = Invalid_Position;

		Value& previous = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;
		Value& control = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;
		Value& end = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;

		mSvg << "<path d =\"M " << Normalize_Time_X(previous.date) << " " << Normalize_Y(previous.value);

		bool ctrEnd = false;

		while (pos != Invalid_Position)
		{
			mSvg << " Q " << Normalize_Time_X(control.date) << " " << Normalize_Y(control.value) << " " << Normalize_Time_X(end.date) << " " << Normalize_Y(end.value);

			// slide by 2 values (so the next bezier curve will start at the end of previous
			previous = end;
			control = findNext(pos, segId);
			if (pos == Invalid_Position)
			{
				ctrEnd = true;
				break;
			}
			end = findNext(pos, segId);
		}

		mSvg << "\"/>" << std::endl;

		if (!ctrEnd)
			mSvg.Line(Normalize_Time_X(previous.date), Normalize_Y(previous.value), Normalize_Time_X(control.date), Normalize_Y(control.value));
	}
}

void CGraph_Generator::Write_Normalized_Lines(ValueVector istVector, ValueVector bloodVector)
{
    mSvg.Set_Stroke(1, "blue", "none");

    // IST curve group scope
    {
        SVG::GroupGuard grp(mSvg, "istCurve", true);
        Write_QuadraticBezierCurve(istVector);
    }

	size_t curColorIdx = 0;

	for (auto& dataVector : mInputData)
	{
		if (dataVector.second.calculated)
		{
			mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], "none");

			// calculated curve group scope
			{
				SVG::GroupGuard grp(mSvg, dataVector.second.identifier + "Curve", true);
				Write_QuadraticBezierCurve(Utility::Get_Value_Vector(mInputData, dataVector.first));
			}

			std::string paramVecName = "param_" + dataVector.second.identifier;
			if (mInputData.find(paramVecName) != mInputData.end())
			{
				SVG::GroupGuard grp(mSvg, paramVecName + "CurveSplit", true);

				for (auto& paramChanges : mInputData[paramVecName].values)
					mSvg.Line_Dash(Normalize_Time_X(paramChanges.date), Normalize_Y(0), Normalize_Time_X(paramChanges.date), Normalize_Y(mMaxValueY));
			}

			curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();
		}
	}

	// segment start/stop markers
	if (mInputData.find("segment_markers") != mInputData.end())
	{
		SVG::GroupGuard grp(mSvg, "SegmentMarkers", true);
		mSvg.Set_Stroke(1, "#CCCCCC", "none");

		for (auto& paramChanges : mInputData["segment_markers"].values)
			mSvg.Line(Normalize_Time_X(paramChanges.date), Normalize_Y(0), Normalize_Time_X(paramChanges.date), Normalize_Y(mMaxValueY));
	}

	// insulin group scope
	if (mInputData.find("insulin") != mInputData.end())
	{
		SVG::GroupGuard grp(mSvg, "insulin", true);

		for (size_t i = 0; i < mInputData["insulin"].values.size(); i++)
		{
			Value& val = mInputData["insulin"].values[i];
			mSvg.Set_Stroke(3, "cyan", "cyan");
			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(0.2) - val.value/2.0, (int)val.value);
		}
	}

	// carbohydrates group scope
	if (mInputData.find("carbs") != mInputData.end())
	{
		SVG::GroupGuard grp(mSvg, "carbs", true);

		for (size_t i = 0; i < mInputData["carbs"].values.size(); i++)
		{
			Value& val = mInputData["carbs"].values[i];
			mSvg.Set_Stroke(3, "green", "green");
			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(1) - val.value / 20.0, (int)(val.value / 10.0));
		}
	}

    mSvg.Set_Default_Stroke();
}

void CGraph_Generator::Write_Description()
{
    double my_y = Normalize_Y(5);
    const double my_x = startX - 15;

    // axis group scope
    {
        SVG::GroupGuard grp(mSvg, "horizontal", false);

        // y-axis

        std::ostringstream rotate;
        rotate << "rotate(-90 " << startX - 30 << "," << maxY / 2 << ")";

        std::stringstream str;
        str << tr("concentration");
        str << (mMmolFlag ? " [mmol/l] " : " [mg/dl] ");
        mSvg.Draw_Text_Transform(0, maxY / 2, str.str(), rotate.str(), "black", 14);
        mSvg.Set_Stroke(2, "black", "none");
        mSvg.Line(startX, Normalize_Y(mMaxValueY), startX, Normalize_Y(0));

        // x-axis
        mSvg.Draw_Text(sizeX / 2, Normalize_Y(0) + 50, tr("time") + "[DD.MM.RRRR HH:mm]", "middle", "black", 14);
        mSvg.Line(startX, Normalize_Y(0), maxX, Normalize_Y(0));

        // horizontal line - value 5

        mSvg.Set_Stroke(1, "grey", "none");

        for (int i = 5; i <= 20; i += 5)
        {
            double my_y = Normalize_Y(i);
            mSvg.Line(startX, my_y, maxX, my_y);
            mSvg.Draw_Text(my_x, my_y, mMmolFlag ? std::to_string(i) : Utility::Format_Decimal(Utility::MmolL_To_MgDl(i), 3), "middle", "grey", 12);
        }
    }

    // hyper/hypoglycemia lines group scope
    {
        SVG::GroupGuard grp(mSvg, "glycemyLine", false);

        mSvg.Set_Stroke(1, "red", "none");

        // hyperglycemia
        my_y = Normalize_Y(5.5);
        mSvg.Draw_Text(startX + 10, my_y - 2, tr("hyperglycemy"), "start", "red", 12);
        mSvg.Line_Dash(startX, my_y, maxX, my_y);

        // hypoglycemia
        my_y = Normalize_Y(3.9);
        mSvg.Draw_Text(startX + 10, my_y - 2, tr("hypoglycemy"), "start", "red", 12);
        mSvg.Line_Dash(startX, my_y, maxX, my_y);
    }

    mSvg.Set_Default_Stroke();
}

void CGraph_Generator::Write_Body()
{
    size_t i;

    Write_Description();
    ValueVector istVector = Utility::Get_Value_Vector(mInputData, "ist");
    ValueVector bloodVector = Utility::Get_Value_Vector(mInputData, "blood");
    Utility::Get_Boundary_Dates(istVector, bloodVector, mMinD, mMaxD);

    // x-axis description
    std::vector<time_t> times = Utility::Get_Times(mMinD, mMaxD);

	mDifD = mMaxD - mMinD;

    // x-axis description group scope
    {
        SVG::GroupGuard grp(mSvg, "vertical", false);

        for (i = 0; i < times.size(); i++)
        {
            time_t t = times[i];
            double x_d = Normalize_Time_X(t);
            mSvg.Set_Stroke(1, "black", "none");
            mSvg.Line(x_d, Normalize_Y(0), x_d, Normalize_Y(0) + 10);

            mSvg.Draw_Text(x_d, Normalize_Y(0) + 30, Utility::Format_Date(t), "middle", "black", 12);
        }
    }

    // blood curve group scope
    {
        SVG::GroupGuard grp(mSvg, "bloodCurve", false);
        for (i = 0; i < bloodVector.size(); i++)
        {
            Value& val = bloodVector[i];
            mSvg.Set_Stroke(3, "red", "red");
            mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(val.value), 3);
        }
    }

    Write_Normalized_Lines(istVector, bloodVector);

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

	mSvg.Set_Stroke(1, "red", "red");

	// blood group scope
	{
		SVG::GroupGuard bloodGrp(mSvg, "blood", false);
		mSvg.Link_Text_color(startX + 10, y, tr("blood"), "change_visibility_blood()", 12);
	}
}

double CGraph_Generator::Normalize_Time_X(time_t date) const
{
    return startX + (((Utility::Time_To_Double(date) - Utility::Time_To_Double(mMinD)) / Utility::Time_To_Double(mDifD)) * (maxX - startX));
}

double CGraph_Generator::Normalize_Y(double val) const
{
    double v = maxY * (val / mMaxValueY) * 0.90;
    // invert axis
    return (maxY - v) - 0.08*maxY;
}

std::string CGraph_Generator::Build_SVG()
{
    mMaxValueY = (mMaxValue > 20) ? mMaxValue : 20;

    mSvg.Header(sizeX, sizeY);
    Write_Body();
    mSvg.Footer();

    return mSvg.Dump();
}

CGraph_Generator::CGraph_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
    : IGenerator(inputData, maxValue, localization, mmolFlag)
{
    mMaxD = 0;
}
