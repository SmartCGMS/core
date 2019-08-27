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

#include "Mobile_Generator.h"

struct TCoordinate;
struct Value;
class Stats;

/*
 * Glucose profile generator class
 */
class CMobile_Carbs_Generator : public CMobile_Generator
{
	private:
		double mMaxValueY;
		std::pair<time_t, time_t> mTimeRange;

	protected:
		// writes plot body
		void Write_Body();
		// writes plot description (axis titles, ..)
		void Write_Description();

		virtual double Normalize_Time_X(time_t x) const override;
		virtual double Normalize_Y(double y) const override;

		// image start X coordinate
		static int startX;
		// image start Y coordinate
		static int startY;
		// image limit X coordinate
		static int sizeX;
		// image limit Y coordinate
		static int sizeY;

	public:
		CMobile_Carbs_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag);

		static void Set_Canvas_Size(int width, int height);

		virtual std::string Build_SVG() override;
};
