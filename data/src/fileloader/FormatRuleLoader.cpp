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

#include "FormatRuleLoader.h"
#include "../../../../../common/SimpleIni.h"
#include "../../../../common/rtl/FilesystemLib.h"

#include "FormatRecognizer.h"
#include "Extractor.h"

#include <codecvt>
#include <locale>

const wchar_t* dsPatternConfigurationFileName = L"patterns.ini";
const wchar_t* dsFormatRuleTemplatesFileName = L"format_rule_templates.ini";
const wchar_t* dsFormatRulesFileName = L"format_rules.ini";

static std::wstring Resolve_Config_File_Path(std::wstring filename)
{
	std::wstring path = Get_Application_Dir();
	Path_Append(path, L"formats");
	Path_Append(path, filename.c_str());

	return path;
}

bool CFormat_Rule_Loader::Load_Format_Pattern_Config()
{
	CSimpleIniA ini;
	SI_Error err;
	ini.SetUnicode();

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
	std::string convPath = converterX.to_bytes(Resolve_Config_File_Path(dsPatternConfigurationFileName));

	err = ini.LoadFile(convPath.c_str());

	if (err < 0)
		return false;

	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);

	size_t counter = 0, rulecounter = 0;
	const char* value;
	for (auto section : sections)
	{
		CSimpleIniA::TNamesDepend keys;
		ini.GetAllKeys(section.pItem, keys);

		for (auto key : keys)
		{
			value = ini.GetValue(section.pItem, key.pItem);
			mFormatRecognizer.Add_Pattern(section.pItem, key.pItem, value);

			rulecounter++;
		}

		counter++;
	}

	return true;
}

bool CFormat_Rule_Loader::Load_Format_Rule_Templates()
{
	CSimpleIniA ini;
	SI_Error err;
	ini.SetUnicode();

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
	std::string convPath = converterX.to_bytes(Resolve_Config_File_Path(dsFormatRuleTemplatesFileName));

	err = ini.LoadFile(convPath.c_str());

	if (err < 0)
		return false;

	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);

	size_t counter = 0;
	const char* value;
	const char* multvalue;
	const char* strformatvalue;
	for (auto section : sections)
	{
		value = ini.GetValue(section.pItem, "replace");
		if (value)
		{
			// we don't load replace rules, these are for anonymizer - we don't anonymize temporarily loaded files
		}

		value = ini.GetValue(section.pItem, "header");
		if (value)
		{
			mExtractor.Add_Template(section.pItem, value);

			multvalue = ini.GetValue(section.pItem, "multiplier");
			if (multvalue)
			{
				try
				{
					double d = std::stod(multvalue);
					mExtractor.Add_Template_Multiplier(section.pItem, value, d);
				}
				catch (...)
				{
					//
				}
			}

			strformatvalue = ini.GetValue(section.pItem, "stringformat");
			if (strformatvalue)
				mExtractor.Add_Template_String_Format(section.pItem, value, strformatvalue);

			counter++;
		}
	}

	return true;
}

bool CFormat_Rule_Loader::Load_Format_Rules()
{
	CSimpleIniA ini;
	SI_Error err;
	ini.SetUnicode();

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
	std::string convPath = converterX.to_bytes(Resolve_Config_File_Path(dsFormatRulesFileName));

	err = ini.LoadFile(convPath.c_str());

	if (err < 0)
		return false;

	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);

	size_t counter = 0, rulecounter = 0;
	bool success;
	const char* value;
	for (auto section : sections)
	{
		CSimpleIniA::TNamesDepend keys;
		ini.GetAllKeys(section.pItem, keys);

		for (auto key : keys)
		{
			value = ini.GetValue(section.pItem, key.pItem);

			success = mExtractor.Add_Format_Rule(section.pItem, key.pItem, value);

			if (success)
				rulecounter++;
		}

		counter++;
	}

	return true;
}

CFormat_Rule_Loader::CFormat_Rule_Loader(CFormat_Recognizer& recognizer, CExtractor& extractor)
	: mFormatRecognizer(recognizer), mExtractor(extractor)
{
	//
}

bool CFormat_Rule_Loader::Load()
{
	return Load_Format_Pattern_Config() &&
		Load_Format_Rule_Templates() &&
		Load_Format_Rules();
}
