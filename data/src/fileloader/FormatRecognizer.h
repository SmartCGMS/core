#pragma once

#include <map>
#include <fstream>
#include <list>

#include "FormatAdapter.h"

using FormatRecognizerRuleMap = std::map<std::string, std::list<std::string>>;

/*
 * Class used to recognize data format (not file format)
 */
class CFormat_Recognizer
{
	private:
		// all rules for all formats; primary key = format name, secondary key = cellspec, value = matched value
		std::map<std::string, FormatRecognizerRuleMap> mRuleSet;

	protected:
		// try to match file contents to data format
		bool Match(std::string format, CFormat_Adapter& file) const;

	public:
		CFormat_Recognizer();

		// adds new format recognition pattern
		void Add_Pattern(const char* formatName, const char* cellLocation, const char* content);

		// recognize data format from supplied path
		std::string Recognize_And_Open(std::wstring path, CFormat_Adapter& target) const;
		// recognize data format from supplied originalPath (to recognize file format as well) and path
		std::string Recognize_And_Open(std::wstring originalPath, std::wstring path, CFormat_Adapter& target) const;
};
