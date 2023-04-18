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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "file_format_detector.h"
#include "Misc.h"

#include <algorithm>
#include <optional>

CFile_Format_Detector::CFile_Format_Detector()
{
	//
}

void CFile_Format_Detector::Add_Pattern(const char* formatName, const char* cellLocation, const char* content)
{
	std::string istr(content);
	size_t offset = 0, base;
	// localized formats - pattern may contain "%%" string to delimit localizations
	while ((base = istr.find("%%", offset)))
	{
		if (base == std::string::npos)
		{
			mRuleSet[formatName][cellLocation].push_back(istr.substr(offset));
			break;
		}

		mRuleSet[formatName][cellLocation].push_back(istr.substr(offset, base - offset));
		offset = base + 2;
	}
}

