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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include "IGenerator.h"

struct TCoordinate;
struct Value;

/*
 * Day statistics generator class
 */
class CDay_Generator : public IGenerator {
	protected:
		// writes main plot body
		void Write_Body();
		// writes set of normalized lines into group
		void Write_Normalized_Lines(ValueVector &values, std::string nameGroup, std::string color);
		// writes quadratic beziere curve
		void Write_QuadraticBezireCurve(std::vector<TCoordinate> values);
		// writes plot legent
		void Write_Legend();
		// writes plot description
		void Write_Description();

		virtual double Normalize_X(int hour, int minute) const override;
		virtual double Normalize_Time_X(time_t x) const override;
		virtual double Normalize_Y(double y) const override;

		// image start X coordinate
		static int startX;
		// image maximum X coordinate (excl. padding)
		static int maxX;
		// image maximum Y coordinate (excl. padding)
		static int maxY;
		// image limit X coordinate
		static int sizeX;
		// image limit Y coordinate
		static int sizeY;

	public:
		CDay_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag);

		static void Set_Canvas_Size(int width, int height);

		virtual std::string Build_SVG() override;
};
