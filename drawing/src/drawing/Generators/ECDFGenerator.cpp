/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "../general.h"
#include "ECDFGenerator.h"

#include "../SVGBuilder.h"
#include "../Misc/Utility.h"

#include <algorithm>
#include <set>
#include <array>
#include <cstring>

int CECDF_Generator::startX = 50;
int CECDF_Generator::maxX = 800;
int CECDF_Generator::maxY = 800;
int CECDF_Generator::sizeX = 800;
int CECDF_Generator::sizeY = 800;

constexpr double Relative_Margin = 0.05;
constexpr std::array<const char*, 4> X_Axis_Uniform_Titles = { { "0.1", "1", "10", "100" } };
constexpr size_t Y_Axis_Step_Count = 11;

void CECDF_Generator::Set_Canvas_Size(int width, int height)
{
	sizeX = width;
	sizeY = height;

	maxX = width;
	maxY = height;
}

void CECDF_Generator::Write_ECDF_Curve(const ValueVector& reference, const ValueVector& calculated, std::string& name)
{
	// we assume the levels are sorted, and that matching levels were calculated properly

	std::vector<double> relErrors;
	std::vector<double> probabilities;

	size_t i = 0, j = 0;

	while (i < reference.size() && j < calculated.size())
	{
		while (i < reference.size() && j < calculated.size() && reference[i].date != calculated[j].date)
		{
			if (reference[i].date > calculated[j].date)
				j++;
			else
				i++;
		}

		if (i >= reference.size() || j >= calculated.size())
			break;

		const double relErr = fabs((calculated[j].value / reference[i].value) - 1.0);
		if (relErr < 1.0)
			relErrors.push_back(relErr);

		i++;
		j++;
	}

	if (relErrors.empty())
		return;

	std::sort(relErrors.begin(), relErrors.end());
	probabilities.resize(relErrors.size());

	// here we have sorted errors, let us calculate 

	for (i = 0; i < relErrors.size(); i++)
	{
		// calculate the cummulative probability
		probabilities[i] = ((double)i / (double)relErrors.size());
		relErrors[i] = std::log(relErrors[i]) / std::log(1000) + 1.0;
	}

	double lastX = 0.0, lastY = 0.0;

	// ECDF curve scope
	{
		SVG::GroupGuard grp(mSvg, name + "EcdfCurve", true);

		for (i = 0; i < relErrors.size(); i++)
		{
			if (relErrors[i] < 0.0)
				continue;

			mSvg.Line(Normalize_X(lastX), Normalize_Y(1.0 - lastY), Normalize_X(relErrors[i]), Normalize_Y(1.0 - lastY));
			mSvg.Line(Normalize_X(relErrors[i]), Normalize_Y(1.0-lastY), Normalize_X(relErrors[i]), Normalize_Y(1.0 - probabilities[i]));

			lastX = relErrors[i];
			lastY = probabilities[i];
		}

		mSvg.Line(Normalize_X(lastX), Normalize_Y(1.0 - lastY), Normalize_X(1.0), Normalize_Y(1.0 - lastY));
		mSvg.Line(Normalize_X(1.0), Normalize_Y(1.0 - lastY), Normalize_X(1.0), Normalize_Y(1.0 - lastY));
	}
}

void CECDF_Generator::Write_Body()
{
	size_t curColorIdx = 0;

	// this block calculates IST using linear interpolation for times of blood samples; this is wrong in many aspects, but:
	// TODO: rework this to generic IST approximation!!! i.e. calculate filter variation
	try
	{
		auto& istVec = Utility::Get_Value_Vector_Ref(mInputData, "ist");
		auto& bloodVec = Utility::Get_Value_Vector_Ref(mInputData, "blood");

		std::vector<Value> toAdd;

		size_t cur_ist = 0;

		if (istVec.size() > 0)
		{
			for (auto& val : bloodVec)
			{
				while (cur_ist < istVec.size() - 1)
				{
					const auto& prevIst = istVec[cur_ist];
					const auto& nextIst = istVec[cur_ist + 1];

					// discard blood values before current ist (the values has to be orderer, so this means we dont have valid ist for it anyway
					if (val.date < prevIst.date)
						break;

					if (val.date > prevIst.date && val.date < nextIst.date)
					{
						const double k = static_cast<double>(nextIst.value - prevIst.value) / static_cast<double>(nextIst.date - prevIst.date);
						const double q = static_cast<double>(prevIst.value);

						if (std::isfinite(k))
							istVec.insert(istVec.begin() + cur_ist + 1, Value{ k*static_cast<double>(val.date) + q, val.date, val.segment_id });
						break;
					}
					else
						cur_ist++;
				}
			}
		}
	}
	catch (...)
	{
		//
	}

	for (auto& dataVector : mInputData)
	{
		if (!dataVector.second.refSignalIdentifier.empty())
		{
			if (dataVector.first == "ist")
				mSvg.Set_Stroke(1, "blue", "none");
			else
				mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], "none");

			// calculated curve group scope
			{
				SVG::GroupGuard grp(mSvg, dataVector.second.identifier + "Curve", true);

				const auto& calc = Utility::Get_Value_Vector(mInputData, dataVector.first);
				const auto& ref = Utility::Get_Value_Vector(mInputData, dataVector.second.refSignalIdentifier);
				if (!calc.empty() && !ref.empty())
					Write_ECDF_Curve(ref, calc, dataVector.second.identifier);
			}

			if (dataVector.first != "ist")
				curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();
		}
	}
}

void CECDF_Generator::Write_Description()
{
	// X axis scope
	{
		SVG::GroupGuard grp(mSvg, "axis_x", false);

		mSvg.Set_Stroke(1, "black", "none");
		mSvg.Line(Normalize_X(0.0), Normalize_Y(1.0), Normalize_X(1.0), Normalize_Y(1.0));

		// X axis titles
		for (size_t i = 0; i < X_Axis_Uniform_Titles.size(); i++)
		{
			const double xpos = (double)i * (1.0 / (double)(X_Axis_Uniform_Titles.size() - 1));
			mSvg.Line(Normalize_X(xpos), Normalize_Y(1.0), Normalize_X(xpos), Normalize_Y(1.0) + 10);

			mSvg.Draw_Text(Normalize_X(xpos) - 3*strlen(X_Axis_Uniform_Titles[i]), Normalize_Y(1.0) + 25, X_Axis_Uniform_Titles[i], "center", "black", 14);
		}
	}

	// Y axis scope
	{
		SVG::GroupGuard grp(mSvg, "axis_y", false);

		mSvg.Set_Stroke(1, "black", "none");
		mSvg.Line(Normalize_X(0.0), Normalize_Y(1.0), Normalize_X(0.0), Normalize_Y(0.0));

		for (size_t i = 0; i < Y_Axis_Step_Count; i++)
		{
			const double ypos = ((double)i * (1.0 / (double)(Y_Axis_Step_Count - 1)));
			mSvg.Line(Normalize_X(0.0), Normalize_Y(ypos), Normalize_X(0.0) - 10, Normalize_Y(ypos));

			mSvg.Draw_Text(Normalize_X(0.0) - 40, Normalize_Y(ypos) + 7, std::to_string((int)round((1.0 - ypos) * 100)), "center", "black", 14);
		}
	}

	// axis group scope
	{
		SVG::GroupGuard grp(mSvg, "axis", true);

		// y-axis
		std::ostringstream rotate;
		double yPom = maxY / 2.0;
		rotate << "rotate(-90 " << startX - 25 << "," << yPom << ")";

		mSvg.Draw_Text_Transform(startX - 40, yPom, tr("cummulative_probability"), rotate.str(), "black", 14);

		// x-axis
		mSvg.Draw_Text(startX + maxX / 2, Normalize_Y(1.0) + 35, tr("rel_error"), "middle", "black", 14);
	}
}

void CECDF_Generator::Write_Legend()
{
	mSvg.Set_Default_Stroke();

	// legend group scope
	{
		SVG::GroupGuard grp(mSvg, "legend", false);

		size_t curColorIdx = 0;
		double y = 55;

		for (auto& dataVector : mInputData)
		{
			if (!dataVector.second.refSignalIdentifier.empty() && !dataVector.second.values.empty())
			{
				if (dataVector.first == "ist")
					mSvg.Set_Stroke(1, "blue", "none");
				else
					mSvg.Set_Stroke(1, Utility::Curve_Colors[curColorIdx], Utility::Curve_Colors[curColorIdx]);

				std::string name = tr(dataVector.second.identifier);

				auto refItr = mInputData.find(dataVector.second.refSignalIdentifier);
				if (refItr != mInputData.end())
					name += " (" + tr(dataVector.second.refSignalIdentifier) + ")";

				SVG::GroupGuard diffGrp(mSvg, dataVector.second.identifier, false);
				mSvg.Link_Text_color(Normalize_X(0.0) + 20, y, name, "", 12); // TODO: add visibility change function

				curColorIdx = (curColorIdx + 1) % Utility::Curve_Colors.size();

				y += 20;
			}
		}

		mSvg.Set_Default_Stroke();
	}
}

double CECDF_Generator::Normalize_X(double x) const
{
	return (double)sizeX * (x*(1.0 - 2.0*Relative_Margin) + Relative_Margin);
}

double CECDF_Generator::Normalize_Y(double y) const
{
	return (double)sizeY * (y*(1.0 - 2.0*Relative_Margin) + Relative_Margin);
}

CECDF_Generator::CECDF_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag)
	: IGenerator(inputData, maxValue, localization, mmolFlag)
{
	//
}

std::string CECDF_Generator::Build_SVG()
{
	mSvg.Header(sizeX, sizeY);
	Write_Body();
	Write_Description();
	Write_Legend();
	mSvg.Footer();

	return mSvg.Dump();
}
