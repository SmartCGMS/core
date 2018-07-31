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
