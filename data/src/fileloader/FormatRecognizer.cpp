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

#include "FormatRecognizer.h"
#include "Misc.h"

#include <algorithm>

CFormat_Recognizer::CFormat_Recognizer()
{
	//
}

void CFormat_Recognizer::Add_Pattern(const char* formatName, const char* cellLocation, const char* content)
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

bool CFormat_Recognizer::Match(std::string format, CFormat_Adapter& file) const
{
	// find format to be matched
	auto itr = mRuleSet.find(format);
	if (itr == mRuleSet.end())
		return false;

	// try all rules, all rules need to be matched
	for (auto const& rulePair : itr->second)
	{
		const std::string rVal = file.Read(rulePair.first.c_str());

		if (!Contains_Element(rulePair.second, rVal) || file.Get_Error() != 0)
			return false;
	}

	return true;
}

std::string CFormat_Recognizer::Recognize_And_Open(std::wstring path, CFormat_Adapter& target) const
{
	return Recognize_And_Open(path, path, target);
}

std::string CFormat_Recognizer::Recognize_And_Open(std::wstring originalPath, std::wstring path, CFormat_Adapter& target) const
{
	// recognize file format at first
	target.Init(path.c_str(), originalPath.c_str());

	if (target.Get_Error() != 0)
		return "";

	// try to recognize data format
	for (auto const& fpair : mRuleSet)
	{
		target.Clear_Error();

		if (Match(fpair.first, target))
			return fpair.first;
	}

	return "";
}
