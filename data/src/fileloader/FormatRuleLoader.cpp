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

#include "FormatRuleLoader.h"
#include "../../../../common/rtl/FilesystemLib.h"
#include "../../../../common/utils/string_utils.h"

#include "FormatRecognizer.h"
#include "Extractor.h"



bool CFormat_Rule_Loader::Load_Format_Config(const char *default_config, const wchar_t* file_name, std::function<bool(CSimpleIniA&)> func) {

	auto resolve_config_file_path = [](const wchar_t* filename) {
		std::wstring path = Get_Application_Dir();
		Path_Append(path, L"formats");
		Path_Append(path, filename);

		return path;
	};


	auto load_config = [&func](const char* str, std::function<SI_Error(const char*, CSimpleIniA&)> load_config_func) {
		CSimpleIniA ini;
		SI_Error err;

		ini.SetUnicode();
		err = load_config_func(str, ini);

		if (err < 0)
			return false;

		return func(ini);
	};

	auto load_config_from_file = [](const char* file_name, CSimpleIniA& ini)->SI_Error {
		return ini.LoadFile(file_name);
	};

	auto load_config_from_memory = [](const char* data, CSimpleIniA& ini)->SI_Error {
		return ini.LoadData(data);
	};

	
	//first, load default configs from memory, then update with file config if they are present
	bool result = load_config(default_config, load_config_from_memory);
	load_config(Narrow_WString(resolve_config_file_path(file_name)).c_str(), load_config_from_file);	//optionally, load external configs
	
	return result;
}




bool CFormat_Rule_Loader::Add_Config_Keys(CSimpleIniA & ini, std::function<void(const char*, const char*, const char*)> func) {
	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);

	for (auto section : sections) {
		CSimpleIniA::TNamesDepend keys;
		ini.GetAllKeys(section.pItem, keys);

		for (auto key : keys) {
			const auto value = ini.GetValue(section.pItem, key.pItem);
			func(section.pItem, key.pItem, value);
		}		
	}

	return true;
}


bool CFormat_Rule_Loader::Load_Format_Pattern_Config(CSimpleIniA& ini) {
	auto add = [this](const char* formatName, const char* cellLocation, const char* content) {
		mFormatRecognizer.Add_Pattern(formatName, cellLocation, content);
	};

	return Add_Config_Keys(ini, add);
}
	

bool CFormat_Rule_Loader::Load_Format_Rules(CSimpleIniA& ini) {
	auto add = [this](const char* formatName, const char* cellLocation, const char* content) {
		mExtractor.Add_Format_Rule(formatName, cellLocation, content);
	};

	return Add_Config_Keys(ini, add);
}



bool CFormat_Rule_Loader::Load_Format_Rule_Templates(CSimpleIniA& ini) {
	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);
	
	const char* value;
	for (auto section : sections) {
		value = ini.GetValue(section.pItem, "replace");
		if (value)
		{
			// we don't load replace rules, these are for anonymizer - we don't anonymize temporarily loaded files
		}

		value = ini.GetValue(section.pItem, "header");
		if (value)
		{
			mExtractor.Add_Template(section.pItem, value);

			const char* multvalue = ini.GetValue(section.pItem, "multiplier");
			if (multvalue)
			{
				try
				{
					const double d = std::stod(multvalue);
					mExtractor.Add_Template_Multiplier(section.pItem, value, d);
				}
				catch (...)
				{
					//
				}
			}

			const char* strformatvalue = ini.GetValue(section.pItem, "stringformat");
			if (strformatvalue)
				mExtractor.Add_Template_String_Format(section.pItem, value, strformatvalue);
		}
	}

	return true;
}

CFormat_Rule_Loader::CFormat_Rule_Loader(CFormat_Recognizer& recognizer, CExtractor& extractor)
	: mFormatRecognizer(recognizer), mExtractor(extractor)
{
	//
}


const wchar_t* dsPatternConfigurationFileName = L"patterns.ini";
const wchar_t* dsFormatRuleTemplatesFileName = L"format_rule_templates.ini";
const wchar_t* dsFormatRulesFileName = L"format_rules.ini";

extern "C" const char default_patterns[];			//bin2c -n default_format_rules_templates -p 0,0 %(FullPath) > %(FullPath).c
extern "C" const char default_format_rules[];
extern "C" const char default_format_rules_templates[];

bool CFormat_Rule_Loader::Load() {	
	
	//order of the following loadings DOES MATTER!
	return Load_Format_Config(default_patterns, dsPatternConfigurationFileName, std::bind(&CFormat_Rule_Loader::Load_Format_Pattern_Config, this, std::placeholders::_1)) &&
		   Load_Format_Config(default_format_rules_templates, dsFormatRuleTemplatesFileName, std::bind(&CFormat_Rule_Loader::Load_Format_Rule_Templates, this, std::placeholders::_1)) &&
		   Load_Format_Config(default_format_rules, dsFormatRulesFileName, std::bind(&CFormat_Rule_Loader::Load_Format_Rules, this, std::placeholders::_1));
}
