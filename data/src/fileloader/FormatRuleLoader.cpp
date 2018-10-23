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

#include "FormatRuleLoader.h"
#include "../../../../common/utils/SimpleIni.h"
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
