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

#include "file_format_rules.h"

#include "../../../../common/rtl/FilesystemLib.h"
#include "../../../../common/utils/string_utils.h"

#include "Extractor.h"

void CFormat_Layout::push(const TCell_Descriptor& cell) {
	mCells.push_back(cell);
}

bool CFile_Format_Rules::Load_Format_Config(const char *default_config, const wchar_t* file_name, std::function<bool(CSimpleIniA&)> func) {

	auto resolve_config_file_path = [](const wchar_t* filename) {
		auto path = Get_Application_Dir();
		path /= L"formats";
		path /= filename;

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
	const std::string path_to_load = resolve_config_file_path(file_name).make_preferred().string();
	load_config(path_to_load.c_str(), load_config_from_file);	//optionally, load external configs
	
	return result;
}




bool CFile_Format_Rules::Add_Config_Keys(CSimpleIniA & ini, std::function<void(const char*, const char*, const char*)> func) {
	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);

	for (const auto &section : sections) {
		CSimpleIniA::TNamesDepend keys;
		ini.GetAllKeys(section.pItem, keys);

		for (const auto &key : keys) {
			const auto value = ini.GetValue(section.pItem, key.pItem);
			func(section.pItem, key.pItem, value);
		}		
	}

	return true;
}


bool CFile_Format_Rules::Load_Format_Pattern_Config(CSimpleIniA& ini) {
	auto add = [this](const char* formatName, const char* cellLocation, const char* content) {
		mFormat_Detection_Rules.Add_Pattern(formatName, cellLocation, content);
	};

	return Add_Config_Keys(ini, add);
}
	

bool CFile_Format_Rules::Load_Format_Layout(CSimpleIniA& ini) {
	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);

	const char* value = nullptr;
	for (const auto& section : sections) {
		const std::string layout_name = section.pItem;

		//check duplicity
		if (mFormat_Layouts.find(layout_name) != mFormat_Layouts.end()) {
			std::wstring msg = L"Found a duplicity format layout, \"";
			msg += Widen_String(layout_name);
			msg += L"\"! Skiping...";
			mErrors.push_back(msg);

			continue;
		}

		CFormat_Layout layout;

		CSimpleIniA::TNamesDepend cell_locations;
		ini.GetAllKeys(section.pItem, cell_locations);

		for (const auto& cell_location : cell_locations) {
			const auto series_name = ini.GetValue(section.pItem, cell_location.pItem);
			if (series_name) {
				const auto series_iter = mSeries.find(series_name);
				if (series_iter != mSeries.end()) {
					TCell_Descriptor cell;
					cell.location = cell_location.pItem;
					cell.series = series_iter->second;
					layout.push(cell);
				}
				else {
					std::wstring msg = L"Format layout, \"";
					msg += Widen_String(layout_name);
					msg += L"\" references a non-exisiting series descriptor \"";
					msg += series_name ? Widen_String(series_name) : L"none_value";
					msg += L"\"!";
					mErrors.push_back(msg);
					continue;
				}			
			}
			else {
				std::wstring msg = L"There is an orphan cell-location\"";
				msg += cell_location.pItem ? Widen_String(cell_location.pItem) : L"none_value";
				msg += L"\" in the format layout  \"";
				msg += Widen_String(layout_name);
				msg += L"\"!";
				mErrors.push_back(msg);
				continue;
			}
		}

		//now, the layout should have all its keys recorded => push it
		mFormat_Layouts[layout_name] = layout;

	}

	return true;	
}


bool CFile_Format_Rules::Load_DateTime_Formats(CSimpleIniA& ini) {
	static const std::string mask_name = "Mask";

	auto add = [this](const char* formatName, const char* iiname, const char* datetime_mask) {
		if (mask_name == iiname)
			mDateTime_Recognizer.push_back(datetime_mask);			
	};

	const auto result = Add_Config_Keys(ini, add);
	if (result)
		mDateTime_Recognizer.finalize_pushes();
	return result;
}


bool CFile_Format_Rules::Load_Series_Descriptors(CSimpleIniA& ini) {
	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);
	
	const char* value = nullptr;
	for (const auto& section : sections) {
		const std::string series_name = section.pItem;
		//check duplicity
		if (mSeries.find(series_name) != mSeries.end()) {
			std::wstring msg = L"Found a duplicity series name, \"";
			msg += Widen_String(series_name);
			msg += L"\"! Skiping...";
			mErrors.push_back(msg);

			continue;
		}

		TSeries_Descriptor desc;
		value = ini.GetValue(section.pItem, "format");
		if (value)
			desc.format = value;

		value = ini.GetValue(section.pItem, "conversion");
		if (value) {
			if (!desc.conversion.init(value)) {
				std::wstring msg = L"Cannot parse \"";
				msg += value ? Widen_String(value) : L"none_value";
				msg += L"\" for time series \"";
				msg += Widen_String(series_name);
				msg += L"\"";
				mErrors.push_back(msg);
				continue;
			}
		}

		bool valid_signal_id = false;
		value = ini.GetValue(section.pItem, "signal");
		if (value)
			desc.target_signal = WString_To_GUID(Widen_String(value), valid_signal_id);
		if (!valid_signal_id) {
			std::wstring msg = L"Cannot convert \"";
			msg += value ? Widen_String(value) : L"none_value";
			msg += L"\" to a valid signal GUID for time series \"";
			msg += Widen_String(series_name);
			msg += L"\"";
			mErrors.push_back(msg);

			continue;
		}


		//once we are here, we have initialized the desc successfully => let's push it
		mSeries[series_name] = std::move(desc);
	}	

	return true;
}

CFile_Format_Rules::CFile_Format_Rules(CFile_Format_Detector& recognizer, CExtractor& extractor, CDateTime_Detector& datetime)
	: mFormat_Detection_Rules(recognizer), mExtractor(extractor), mDateTime_Recognizer(datetime)
{
	//
}


const wchar_t* dsFormat_Detection_Configuration_Filename = L"format_detection.ini";
const wchar_t* dsSeries_Definitions_Filename = L"format_series.ini";
const wchar_t* dsFormat_Layout_Filename = L"format_layout.ini";
const wchar_t* dsDateTime_Formats_FileName = L"datetime_formats.ini";

extern "C" const char default_format_detection[];			//bin2c -n default_format_rules_templates -p 0,0 %(FullPath) > %(FullPath).c
extern "C" const char default_format_layout[];
extern "C" const char default_format_series[];
extern "C" const char default_datetime_formats[];

bool CFile_Format_Rules::Load() {	
	
	//order of the following loadings DOES MATTER!
	return Load_Format_Config(default_format_detection, dsFormat_Detection_Configuration_Filename, std::bind(&CFile_Format_Rules::Load_Format_Pattern_Config, this, std::placeholders::_1)) &&
		   Load_Format_Config(default_format_series, dsSeries_Definitions_Filename, std::bind(&CFile_Format_Rules::Load_Series_Descriptors, this, std::placeholders::_1)) &&
		   Load_Format_Config(default_format_layout, dsFormat_Layout_Filename, std::bind(&CFile_Format_Rules::Load_Format_Layout, this, std::placeholders::_1)) &&
		   Load_Format_Config(default_datetime_formats, dsDateTime_Formats_FileName, std::bind(&CFile_Format_Rules::Load_DateTime_Formats, this, std::placeholders::_1));
}
