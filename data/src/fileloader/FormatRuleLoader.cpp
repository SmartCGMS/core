#include "FormatRuleLoader.h"
#include "../../../../../common/SimpleIni.h"
#include "../../../../common/rtl/FilesystemLib.h"

#include "FormatRecognizer.h"
#include "Extractor.h"

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

	err = ini.LoadFile(Resolve_Config_File_Path(dsPatternConfigurationFileName).c_str());

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

	err = ini.LoadFile(Resolve_Config_File_Path(dsFormatRuleTemplatesFileName).c_str());

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

	err = ini.LoadFile(Resolve_Config_File_Path(dsFormatRulesFileName).c_str());

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
