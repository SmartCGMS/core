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

#pragma once

#include "../../../../common/rtl/referencedImpl.h"
#include "../../../../common/rtl/hresult.h"
#include "../../../../common/rtl/FilesystemLib.h"
#include "../../../../common/utils/SimpleIni.h"


#include "value_convertor.h"
#include "time_utils.h"

#include <string>
#include <map>
#include <optional>

	//TSeries_Descriptor gives a unique
struct TSeries_Descriptor {
	std::string comment_name;
	std::string datetime_format;		//string format used to extract the values
	GUID target_signal = Invalid_GUID;
	CValue_Convertor conversion;		//we support expressions to e.g.; make Fahrenheit to Celsius converion easy	
};


struct TCell_Descriptor {
	std::string cursor_position;
	TSeries_Descriptor series;
};

class CFormat_Layout {
protected:
	std::vector<TCell_Descriptor> mCells;
public:
	void push(const TCell_Descriptor& cell);
	bool empty();

	std::vector<TCell_Descriptor>::iterator begin() { return mCells.begin(); };
	std::vector<TCell_Descriptor>::iterator end() { return mCells.end(); };
};



using TFormat_Signature_Map = std::map<std::string, std::list<std::string>>;
using TFormat_Signature_Rules = std::map<std::string, TFormat_Signature_Map>;

class CFile_Format_Rules {
protected:
	TFormat_Signature_Rules mFormat_Signatures; // all rules for all formats; primary key = format name, secondary key = cellspec, value = matched value

	std::map<std::string, TSeries_Descriptor> mSeries;			//organized as <series_name, desc>
	std::map<std::string, CFormat_Layout> mFormat_Layouts;		//organized as <format_name, cells info>

	bool Load_Format_Definition(CSimpleIniA& ini);
	bool Load_Series_Descriptors(CSimpleIniA& ini);	
	
    bool Add_Config_Keys(CSimpleIniA& ini, std::function<void(const char*, const char*, const char*)> func);
    bool Load_Format_Config(const char* default_config, const wchar_t* file_name, std::function<bool(CSimpleIniA&)> func);
	
	
	TFormat_Signature_Map Load_Format_Signature(const CSimpleIniA& ini, const CSimpleIniA::Entry& section);
	CFormat_Layout Load_Format_Layout(const CSimpleIniA& ini, const std::string &section_name);
protected:
	std::vector<std::wstring> mErrors;		//container for errors encountered during parsing
	bool mValid = false;
	bool Load();
public:
	CFile_Format_Rules();

	bool Are_Rules_Valid(refcnt::Swstr_list& error_description) const;	//pushes any error occured during the load and returns mValid
	TFormat_Signature_Rules Signature_Rules() const;	
	std::optional<CFormat_Layout> Format_Layout(const std::string& format_name) const;	
	bool Load_Additional_Format_Layout(const filesystem::path& path);
};