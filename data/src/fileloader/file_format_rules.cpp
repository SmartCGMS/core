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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "file_format_rules.h"

#include <scgms/rtl/FilesystemLib.h>
#include <scgms/utils/string_utils.h>

namespace {
	const wchar_t* dsSeries_Definitions_Filename = L"format_series.ini";
	const wchar_t* dsFormat_Layout_Filename = L"format_layout.ini";
}

extern "C" const char default_format_layout[];			//bin2c -n default_format_rules_templates -p 0,0 %(FullPath) > %(FullPath).c
extern "C" const char default_format_series[];

void CFormat_Layout::push(const TCell_Descriptor& cell) {
	mCells.push_back(cell);
}

bool CFormat_Layout::empty() {
	return mCells.empty();
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

		if (err < 0) {
			return false;
		}

		return func(ini);
	};

	auto load_config_from_file = [](const char* file_name, CSimpleIniA& ini)->SI_Error {
		return ini.LoadFile(file_name);
	};

	auto load_config_from_memory = [](const char* data, CSimpleIniA& ini)->SI_Error {
		return ini.LoadData(data);
	};

	bool default_result = false;
	if (default_config != nullptr) {
		default_result = load_config(default_config, load_config_from_memory);
	}

	if (file_name != nullptr) {
		const std::string path_to_load = resolve_config_file_path(file_name).make_preferred().string();
		const bool file_result = load_config(path_to_load.c_str(), load_config_from_file);	//optionally, load external configs
		if ((default_config != nullptr) && (default_result)) {
			//with succesfully loaded default config, file result does not matter
		}
		else {
			default_result = file_result;
		}
	}

	return default_result;
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

bool CFile_Format_Rules::Load_Format_Definition(CSimpleIniA& ini) {
	const std::string signature_suffix = ".signature";
	const std::string cursors_suffix = ".cursors";
	
	CSimpleIniA::TNamesDepend sections;
	ini.GetAllSections(sections);

	for (auto& section : sections) {
		const std::string section_name = section.pItem;
		if (ends_with(section_name, signature_suffix)) {
			const std::string format_name = section_name.substr(0, section_name.size() - signature_suffix.size());
			const std::string cursors_section_name = format_name + cursors_suffix;

			auto signature = Load_Format_Signature(ini, section);
			auto layout = Load_Format_Layout(ini, cursors_section_name);

			if (!signature.empty() && !layout.empty()) {

				auto check_override = [this](auto container, const std::string& format_name) {
					auto iter = container.find(format_name);
					if (iter != container.end()) {
						std::wstring msg = L"Warning: Overriding ";
						msg += Widen_String(format_name);
						msg += L" with new definition!";
						mErrors.push_back(msg);
					}
				};

				check_override(mFormat_Signatures, format_name);
				check_override(mFormat_Layouts, format_name);

				mFormat_Signatures[format_name] = signature;
				mFormat_Layouts[format_name] = layout;
			}
			else {
				std::wstring msg = L"Warning: Format ";
				msg += Widen_String(format_name);
				msg += L" was not loaded! Check if you have defined both signatures and cursors properly.";
				mErrors.push_back(msg);
			}

		}
	}

	//note that the default ini file could be empty and all formats could be specified later, in a separate file
	//=>we just return true
	return true;
}

TFormat_Signature_Map CFile_Format_Rules::Load_Format_Signature(const CSimpleIniA& ini, const CSimpleIniA::Entry& section) {
	TFormat_Signature_Map result;

	CSimpleIniA::TNamesDepend signature_cursors;
	ini.GetAllKeys(section.pItem, signature_cursors);

	for (const auto& cursors_position : signature_cursors) {
		const auto value = ini.GetValue(section.pItem, cursors_position.pItem);

		std::string istr(value);
		size_t offset = 0;
		std::list<std::string> localized_names;

		// localized formats - pattern may contain "%%" string to delimit localizations
		const std::string localization_delimiter = "%%";
		while (offset < istr.size()) {
			size_t base = istr.find(localization_delimiter, offset);

			if (base == std::string::npos) {
				const auto substr = istr.substr(offset);
				const auto trimmed = trim(substr);
				localized_names.push_back(trimmed);
				break;
			}

			localized_names.push_back(trim(istr.substr(offset, base - offset)));
			offset = base + localization_delimiter.size();
		}

		result[cursors_position.pItem] = localized_names;
	}

	return result;
}


CFormat_Layout CFile_Format_Rules::Load_Format_Layout(const CSimpleIniA& ini, const std::string& section_name) {
	CFormat_Layout layout;
	CSimpleIniA::TNamesDepend cursor_init_positions;
	ini.GetAllKeys(section_name.c_str(), cursor_init_positions);

	for (const auto& cursor_position : cursor_init_positions) {
		const auto series_name = ini.GetValue(section_name.c_str(), cursor_position.pItem);
		if (series_name) {
			const auto series_iter = mSeries.find(series_name);
			if (series_iter != mSeries.end()) {
				TCell_Descriptor cell;
				cell.cursor_position = trim(cursor_position.pItem);
				cell.series = series_iter->second;
				layout.push(cell);
			}
			else {
				std::wstring msg = L"Format layout, \"";
				msg += Widen_String(section_name.c_str());
				msg += L"\" references a non-exisiting series descriptor \"";
				msg += series_name ? Widen_String(series_name) : L"none_value";
				msg += L"\"!";
				mErrors.push_back(msg);
				continue;
			}
		}
		else {
			std::wstring msg = L"There is an orphan cursor position\"";
			msg += cursor_position.pItem ? Widen_String(cursor_position.pItem) : L"none_value";
			msg += L"\" in the format layout  \"";
			msg += Widen_String(section_name);
			msg += L"\"!";
			mErrors.push_back(msg);
			continue;
		}
	}

	return layout;
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
		value = ini.GetValue(section.pItem, "datetime_format");
		if (value) {
			desc.datetime_format = trim(value);
		}

		value = ini.GetValue(section.pItem, "comment_name");
		if (value) {
			desc.comment_name = trim(value);
		}

		value = ini.GetValue(section.pItem, "conversion");
		if (value) {
			if (!desc.conversion.init(trim(value))) {
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
		if (value) {
			desc.target_signal = WString_To_GUID(Widen_String(value), valid_signal_id);
		}

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

TFormat_Signature_Rules CFile_Format_Rules::Signature_Rules() const {
	return mFormat_Signatures;
}

std::optional<CFormat_Layout> CFile_Format_Rules::Format_Layout(const std::string& format_name) const {
	auto iter = mFormat_Layouts.find(format_name);
	if (iter != mFormat_Layouts.end()) {
		return iter->second;
	}
	else {
		return std::nullopt;
	}
}

CFile_Format_Rules::CFile_Format_Rules() {
	mValid = Load();
}

bool CFile_Format_Rules::Are_Rules_Valid(refcnt::Swstr_list& error_description) const {
	for (const auto& err : mErrors) {
		error_description.push(err);
	}
	return mValid;
}

bool CFile_Format_Rules::Load() {
	
	//order of the following loadings DOES MATTER!
	return Load_Format_Config(default_format_series, dsSeries_Definitions_Filename, std::bind(&CFile_Format_Rules::Load_Series_Descriptors, this, std::placeholders::_1)) &&
		   Load_Format_Config(default_format_layout, dsFormat_Layout_Filename, std::bind(&CFile_Format_Rules::Load_Format_Definition, this, std::placeholders::_1));
}

bool CFile_Format_Rules::Load_Additional_Format_Layout(const filesystem::path& path) {
	CSimpleIniA ini;

	ini.SetUnicode();

	if (ini.LoadFile(path.c_str()) != SI_Error::SI_OK) {
		return false;
	}

	return Load_Format_Definition(ini);
}
