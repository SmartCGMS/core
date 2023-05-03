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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "../general.h"
#include "GraphGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/Utility.h"
#include "../../../../../common/utils/string_utils.h"
#include "../../../../../common/utils/DebugHelper.h"

#include <algorithm>
#include <set>

#undef min
#undef max

constexpr size_t Invalid_Position = std::numeric_limits<size_t>::max();

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

void CGraph_Generator::Write_QuadraticBezierCurve(const ValueVector& values)
{
	if (values.empty())
		return;

	auto findNext = [&values](size_t &pos, uint64_t segId) -> size_t {
		// the unsigned overflow is well defined, so if "pos" equals Invalid_Position (-1), it will overflow to 0
		for (pos++; pos < values.size(); pos++)
		{
			if (values[pos].segment_id == segId)
				return pos;
		}

		pos = Invalid_Position;
		return Invalid_Position;
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

		size_t previous = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;
		size_t control = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;
		size_t end = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;

		mSvg << "<path d =\"M " << Normalize_Time_X(values[previous].date) << " " << Normalize_Y(values[previous].value);

		bool ctrEnd = false;

		while (pos != Invalid_Position)
		{
			mSvg << " Q " << Normalize_Time_X(values[control].date) << " " << Normalize_Y(values[control].value) << " " << Normalize_Time_X(values[end].date) << " " << Normalize_Y(values[end].value);

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
			mSvg.Line(Normalize_Time_X(values[previous].date), Normalize_Y(values[previous].value), Normalize_Time_X(values[control].date), Normalize_Y(values[control].value));
	}
}

void CGraph_Generator::Write_LinearCurve(const ValueVector& values, const Data& dataMeta)
{
	if (values.empty())
		return;

	auto findNext = [&values](size_t &pos, uint64_t segId) -> size_t {
		// the unsigned overflow is well defined, so if "pos" equals Invalid_Position (-1), it will overflow to 0
		for (pos++; pos < values.size(); pos++)
		{
			if (values[pos].segment_id == segId)
				return pos;
		}

		pos = Invalid_Position;
		return Invalid_Position;
	};

	// extract segment IDs from values
	std::set<uint64_t> segmentIds;
	for (auto& val : values)
		segmentIds.insert(val.segment_id);

	// draw segment lines separatelly
	for (uint64_t segId : segmentIds)
	{
		size_t pos = Invalid_Position, next;

		size_t previous = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;

		mSvg << "<path d =\"M " << Normalize_Time_X(values[previous].date) << " " << Normalize_Y(values[previous].value * dataMeta.valuesScale);

		while (pos != Invalid_Position)
		{
			next = findNext(pos, segId);
			if (next == Invalid_Position)
				break;

			// disallow visual linear approximation of more than 20min gaps; split the line if the gap is bigger
			if (values[next].date - values[previous].date > 20 * 60)
			{
				mSvg << "\"/>" << std::endl;
				mSvg << "<path d =\"M " << Normalize_Time_X(values[next].date) << " " << Normalize_Y(values[next].value * dataMeta.valuesScale);
			}

			mSvg << " L " << Normalize_Time_X(values[next].date) << " " << Normalize_Y(values[next].value * dataMeta.valuesScale);
			pos = next;
			previous = next;
		}

		mSvg << "\"/>" << std::endl;
	}
}

void CGraph_Generator::Write_StepCurve(const ValueVector& values, const Data& dataMeta)
{
	if (values.empty())
		return;

	auto findNext = [&values](size_t &pos, uint64_t segId) -> size_t {
		// the unsigned overflow is well defined, so if "pos" equals Invalid_Position (-1), it will overflow to 0
		for (pos++; pos < values.size(); pos++)
		{
			if (values[pos].segment_id == segId)
				return pos;
		}

		pos = Invalid_Position;
		return Invalid_Position;
	};

	// extract segment IDs from values
	std::set<uint64_t> segmentIds;
	for (auto& val : values)
		segmentIds.insert(val.segment_id);

	// draw segment lines separatelly
	for (uint64_t segId : segmentIds)
	{
		size_t pos = Invalid_Position, next;

		size_t previous = findNext(pos, segId);
		if (pos == Invalid_Position)
			continue;

		mSvg << "<path d =\"M " << Normalize_Time_X(values[previous].date) << " " << Normalize_Y(0);

		mSvg << " L " << Normalize_Time_X(values[previous].date) << " " << Normalize_Y(values[previous].value * dataMeta.valuesScale);

		while (pos != Invalid_Position)
		{
			next = findNext(pos, segId);
			if (next == Invalid_Position)
				break;

			mSvg << " L " << Normalize_Time_X(values[next].date) << " " << Normalize_Y(values[previous].value * dataMeta.valuesScale);
			mSvg << " L " << Normalize_Time_X(values[next].date) << " " << Normalize_Y(values[next].value * dataMeta.valuesScale);
			pos = next;
			previous = next;
		}

		mSvg << "\"/>" << std::endl;
	}
}

void CGraph_Generator::Write_Normalized_Lines(ValueVector istVector, ValueVector bloodVector)
{
    mSvg.Set_Stroke(1, "blue", "none");

    // IST curve group scope
    {
        SVG::GroupGuard grp(mSvg, "istCurve", true);
		Write_LinearCurve(istVector, mInputData["ist"]);
    }

	// IoB curve group scope
	try
	{
		ValueVector& iobs = Utility::Get_Value_Vector_Ref(mInputData, "iob");

		mSvg.Set_Stroke(1, "#5555DD", "none");
		SVG::GroupGuard grp(mSvg, "iobCurve", true);
		Write_LinearCurve(iobs, mInputData["iob"]);
	}
	catch (...)
	{
		//
	}

	// heartbeat curve group scope
	try
	{
		ValueVector& bpms = Utility::Get_Value_Vector_Ref(mInputData, "heartbeat");

		if (!bpms.empty()) {

			auto& bpmsData = mInputData["heartbeat"];
			bpmsData.valuesScale = 0.1; // force

			mSvg.Set_Stroke(1, "#00DD00", "none");
			SVG::GroupGuard grp(mSvg, "heartbeat", true);
			Write_LinearCurve(bpms, bpmsData);
		}
	}
	catch (...)
	{
		//
	}

	// CoB curve group scope
	try
	{
		ValueVector& cobs = Utility::Get_Value_Vector_Ref(mInputData, "cob");

		if (!cobs.empty()) {

			auto& cobData = mInputData["cob"];
			cobData.valuesScale = 0.1; // force

			mSvg.Set_Stroke(1, "#55DD55", "none");
			SVG::GroupGuard grp(mSvg, "cobCurve", true);
			Write_LinearCurve(cobs, cobData);
		}
	}
	catch (...)
	{
		//
	}

	// suggested basal curve group scope
	try
	{
		ValueVector& rates = Utility::Get_Value_Vector_Ref(mInputData, "basal_insulin_rate");

		if (!rates.empty()) {
			mSvg.Set_Stroke(1, "#55DDDD", "none");
			SVG::GroupGuard grp(mSvg, "basal_insulin_rateCurve", true);
			Write_StepCurve(rates, mInputData["basal_insulin_rate"]);
		}
	}
	catch (...)
	{
		//
	}

	// insulin activity curve group scope
	try
	{
		ValueVector& insact = Utility::Get_Value_Vector_Ref(mInputData, "insulin_activity");

		mSvg.Set_Stroke(1, "#DD5500", "none");
		SVG::GroupGuard grp(mSvg, "insulin_activity", true);
		Write_LinearCurve(insact, mInputData["insulin_activity"]);
	}
	catch (...)
	{
		//
	}

	size_t curColorIdx = 0;

	for (auto& dataVector : mInputData)
	{
		if ((dataVector.second.calculated || dataVector.second.forceDraw) && !dataVector.second.empty)
		{
			mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], "none");

			// calculated curve group scope
			{
				SVG::GroupGuard grp(mSvg, dataVector.second.identifier + "Curve", true);
				if (dataVector.second.visualization_style == scgms::NSignal_Visualization::step)
					Write_StepCurve(Utility::Get_Value_Vector(mInputData, dataVector.first), dataVector.second);
				else
					Write_LinearCurve(Utility::Get_Value_Vector(mInputData, dataVector.first), dataVector.second);
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
			mSvg.Set_Stroke(3, "green", "none");
			//mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(1) - val.value / 20.0, (int)(val.value / 10.0));
			//mSvg.Point(Normalize_Time_X(val.date), (int)(Normalize_Y(val.value / 10.0)), (int)(val.value / 10.0));
			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(val.value/10), (int)(val.value/5.0));
		}
	}

	// calculated bolus insulin group scope
	if (mInputData.find("calcd_insulin") != mInputData.end())
	{
		auto vec = Utility::Get_Value_Vector_Ref(mInputData, "calcd_insulin");

		SVG::GroupGuard grp(mSvg, "bloodCurve", false);
		for (size_t i = 0; i < vec.size(); i++)
		{
			Value& val = vec[i];
			if (val.value > 0.01) // ignore zero (or near-zero) boluses
			{
				mSvg.Set_Stroke(5, "#8855CC", "#8855CC");
				mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(val.value), 3);
			}
		}
	}

    mSvg.Set_Default_Stroke();
}

void CGraph_Generator::Write_Description()
{    
    const double my_x = startX - 15;

    // axis group scope
    {
        SVG::GroupGuard grp(mSvg, "horizontal", false);

        // y-axis

        std::ostringstream rotate;
        rotate << "rotate(-90 " << startX - 30 << "," << maxY / 2 << ")";

        std::stringstream str;
        str << tr("concentration");
        str << (mMmolFlag ? " [mmol/L] " : " [mg/dL] ");
		str << ", Insulin[U], Insulin Rate[U/Hr], Carbohydrates[0.1*g] ";
        mSvg.Draw_Text_Transform(0, maxY / 2, str.str(), rotate.str(), "black", 14);
        mSvg.Set_Stroke(2, "black", "none");
        mSvg.Line(startX, Normalize_Y(mMaxValueY), startX, Normalize_Y(0));

        // x-axis
        mSvg.Draw_Text(sizeX / 2, Normalize_Y(0) + 50, tr("time") /*+ "[DD.MM.RRRR HH:mm]"*/, "middle", "black", 14);
        mSvg.Line(startX, Normalize_Y(0), maxX, Normalize_Y(0));

        // horizontal line - step by 2

        mSvg.Set_Stroke(1, "#CCCCCC", "none");

        for (int i = 2; i <= std::max(20, (int)mMaxValueY); i += 2)
        {
            double my_y = Normalize_Y(i);
            mSvg.Line(startX, my_y, maxX, my_y);
			mSvg.Draw_Text(my_x, my_y, mMmolFlag ? std::to_string(i) : Utility::Format_Decimal(Utility::MmolL_To_MgDl(i), 3), "middle", "grey", 12);
        }
    }

    // hyper/hypoglycemia lines group scope
    {
        SVG::GroupGuard grp(mSvg, "glycemyLine", false);

        mSvg.Set_Stroke(1, "red", "none", 0.6, 0.6);

        // hyperglycemia
        double my_y = Normalize_Y(11.0);
		mSvg.Draw_Text(startX + 5, my_y - 5, tr("hyperglycemia"), "start", "red", 12);
        mSvg.Line_Dash(startX, my_y, maxX, my_y);

		mSvg.Draw_Text(my_x, my_y, mMmolFlag ? "11" : Utility::Format_Decimal(Utility::MmolL_To_MgDl(11.0), 3), "middle", "grey", 12);

        // hypoglycemia
        my_y = Normalize_Y(3.5);
        mSvg.Draw_Text(startX + 5, my_y + 13, tr("hypoglycemia"), "start", "red", 12);
        mSvg.Line_Dash(startX, my_y, maxX, my_y);

		mSvg.Draw_Text(my_x, my_y, mMmolFlag ? "3.5" : Utility::Format_Decimal(Utility::MmolL_To_MgDl(3.5), 3), "middle", "grey", 12);

		
		mSvg.Set_Stroke(1, "#FF8888", "none", 0.6, 0.6);

		// elevated glycemia
		my_y = Normalize_Y(5.5);
		mSvg.Draw_Text(startX + 5, my_y - 5, tr("elevatedglucose"), "start", "#FF8888", 12);
		mSvg.Line_Dash(startX, my_y, maxX, my_y);

		mSvg.Draw_Text(my_x, my_y, mMmolFlag ? "5.5" : Utility::Format_Decimal(Utility::MmolL_To_MgDl(5.5), 3), "middle", "grey", 12);
    }

    mSvg.Set_Default_Stroke();
}

void CGraph_Generator::Write_Body()
{
    size_t i;

    Write_Description();
    ValueVector istVector = Utility::Get_Value_Vector(mInputData, "ist");
    ValueVector bloodVector = Utility::Get_Value_Vector(mInputData, "blood");
	ValueVector bloodCalibrationVector = Utility::Get_Value_Vector(mInputData, "bloodCalibration");

	mMaxD = std::numeric_limits<decltype(mMaxD)>::min();
	mMinD = std::numeric_limits<decltype(mMinD)>::max();

	for (const auto &iter : mInputData) {
		const auto &series = iter.second.values;
		if (series.empty()) continue;
		if (!iter.second.visible || !iter.second.calculated) continue;

		if (mMinD > series[0].date) mMinD = series[0].date;		
		if (mMaxD < series[series.size()-1].date) 
			mMaxD = series[series.size()-1].date;
	}
	

    // x-axis description
    std::vector<time_t> times = Utility::Get_Times(mMinD, mMaxD);
	
	const time_t one_day = 24 * 60 * 60;
	const time_t ten_days = 10 * one_day;
	mDifD = mMaxD - mMinD;
	if (mDifD > ten_days)
		sizeX = maxX = static_cast<decltype(sizeX)>(mDifD / one_day) * 100;

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

	// blood curve group scope
	{
		SVG::GroupGuard grp(mSvg, "bloodCalibrationCurve", false);
		for (i = 0; i < bloodCalibrationVector.size(); i++)
		{
			Value& val = bloodCalibrationVector[i];
			mSvg.Set_Stroke(3, "#EC80FF", "#EC80FF");
			mSvg.Point(Normalize_Time_X(val.date), Normalize_Y(val.value), 3);
		}
	}


	size_t curColorIdx = 0;
	double y = 40;

	// ist legend group scope
	if (!istVector.empty())
	{
		SVG::GroupGuard istGrp(mSvg, "ist", false);
		mSvg.Set_Stroke(1, "blue", "none");
		mSvg.Link_Text_color(startX + 10, y, tr("ist"), "change_visibility_ist()", 12);

		y += 20;
	}

	for (auto& dataVector : mInputData)
	{
		if ((dataVector.second.calculated || dataVector.second.forceDraw) && !dataVector.second.empty)
		{
			mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], Utility::Curve_Colors[curColorIdx]);

			const std::string& name = dataVector.second.nameAlias.empty() ? tr(dataVector.second.identifier) : Narrow_WString(dataVector.second.nameAlias);
			SVG::GroupGuard diffGrp(mSvg, dataVector.second.identifier, false);
			mSvg.Link_Text_color(startX + 10, y, name, "", 12); // TODO: add visibility change function

			curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();

			y += 20;
		}
	}

	mSvg.Set_Stroke(1, "red", "red");

	// blood group scope
	if (!bloodVector.empty())
	{
		SVG::GroupGuard bloodGrp(mSvg, "blood", false);
		mSvg.Link_Text_color(startX + 10, y, tr("blood"), "change_visibility_blood()", 12);

		y += 20;
	}

	mSvg.Set_Stroke(1, "#EC80FF", "#EC80FF");

	// blood calibration group scope
	if (!bloodCalibrationVector.empty())
	{
		SVG::GroupGuard bloodCalibrationGrp(mSvg, "bloodCalibration", false);
		mSvg.Link_Text_color(startX + 10, y, tr("bloodCalibration"), "change_visibility_bloodCalibration()", 12);

		y += 20;
	}

	mSvg.Set_Stroke(1, "green", "green");

	// carb intake group scope
	if (mInputData.find("carbs") != mInputData.end())
	{
		SVG::GroupGuard bloodCalibrationGrp(mSvg, "carbIntake", false);
		mSvg.Link_Text_color(startX + 10, y, tr("carbIntake"), "change_visibility_bloodCalibration()", 12);

		y += 20;
	}

	mSvg.Set_Stroke(1, "#5555DD", "#5555DD");

	// IOB group scope
	if (mInputData.find("iob") != mInputData.end())
	{
		SVG::GroupGuard bloodCalibrationGrp(mSvg, "iob", false);
		mSvg.Link_Text_color(startX + 10, y, tr("iob"), "", 12);

		y += 20;
	}

	mSvg.Set_Stroke(1, "#DD5500", "#DD5500");

	// IOB group scope
	if (mInputData.find("insulin_activity") != mInputData.end())
	{
		SVG::GroupGuard bloodCalibrationGrp(mSvg, "insulin_activity", false);
		mSvg.Link_Text_color(startX + 10, y, tr("insulin_activity"), "", 12);

		y += 20;
	}

	mSvg.Set_Stroke(1, "#55DD55", "#55DD55");

	// COB group scope
	if (mInputData.find("cob") != mInputData.end())
	{
		SVG::GroupGuard bloodCalibrationGrp(mSvg, "cob", false);
		mSvg.Link_Text_color(startX + 10, y, tr("cob"), "", 12);

		y += 20;
	}

	mSvg.Set_Stroke(1, "#55DDDD", "#55DDDD");

	// suggested basal rate group scope
	if (mInputData.find("basal_insulin_rate") != mInputData.end())
	{
		SVG::GroupGuard bloodCalibrationGrp(mSvg, "basal_insulin_rate", false);
		mSvg.Link_Text_color(startX + 10, y, tr("basal_insulin_rate"), "", 12);

		y += 20;
	}

	mSvg.Set_Stroke(1, "#8855DD", "#8855DD");

	// calculated bolus dosage group scope
	if (mInputData.find("calcd_insulin") != mInputData.end())
	{
		SVG::GroupGuard bloodCalibrationGrp(mSvg, "calcd_insulin", false);
		mSvg.Link_Text_color(startX + 10, y, tr("calcd_insulin"), "", 12);

		//y += 20; - no more used
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
