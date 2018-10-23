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
 *       monitoring", Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 */

#pragma once

#include "IGenerator.h"

#include <array>

struct TCoordinate;

/*
 * Parkes generator class
 */
class CParkes_Generator : public IGenerator
{
    protected:
        // is diabetes type 1? true = type 1, false = type 2
        bool mDiabetesType1;

        // writes main plot body
        void Write_Body();
        // writes plot description (axis titles, ...)
        void Write_Description();
        // writes single legend item
        void Write_Legend_Item(std::string id, std::string text, std::string function, double y);
        // writes background map for diabetes type 1
        void Write_Background_Map_Type1();
        // writes background map for diabetes type 2
        void Write_Background_Map_Type2();
        // writes normalized set of lines
		template <typename T>
		void Write_Normalized_Lines(const T &values) {
			auto &start = values[0];

			mSvg << "<path d =\"M " << Normalize_X(start.x) << " " << Normalize_Y(start.y);

			for (unsigned int i = 1; i < values.size(); i++)
				mSvg << " L " << Normalize_X(values[i].x) << " " << Normalize_Y(values[i].y);

			mSvg << "\"/>" << std::endl;
		}

        virtual double Normalize_X(double x) const override;
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
        CParkes_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag, bool diabetesType1);

        static void Set_Canvas_Size(int width, int height);

        virtual std::string Build_SVG() override;
};
