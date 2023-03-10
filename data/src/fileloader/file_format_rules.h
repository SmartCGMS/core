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

#pragma once

#include "../../../../common/rtl/hresult.h"
#include "../../../../common/utils/SimpleIni.h"


#include "file_format_detector.h"
#include "value_convertor.h"
#include "Extractor.h"

#include <string>
#include <map>

	//TSeries_Descriptor gives a unique
struct TSeries_Descriptor {
	std::string format;				//id and string format used to extract the values
	GUID target_signal;
	CValue_Convertor conversion;		//we support expressions to e.g.; make Fahrenheit to Celsius converion easy	
};


struct TCell_Descriptor {
	std::string location;
	TSeries_Descriptor series;
};

class CFormat_Layout {
protected:
	std::vector<TCell_Descriptor> mCells;
public:
	void push(const TCell_Descriptor& cell);
};

class CFile_Format_Rules {
protected:
	CFile_Format_Detector& mFormat_Detection_Rules;
	CDateTime_Detector& mDateTime_Recognizer;


	std::map<std::string, TSeries_Descriptor> mSeries;	//organized as <id, desc>
	std::map<std::string, CFormat_Layout> mFormat_Layouts;		//organized as <format_name, cells info>

	bool Load_Format_Pattern_Config(CSimpleIniA& ini);
	bool Load_Series_Descriptors(CSimpleIniA& ini);
	bool Load_Format_Layout(CSimpleIniA& ini);
	bool Load_DateTime_Formats(CSimpleIniA& ini);

    bool Add_Config_Keys(CSimpleIniA& ini, std::function<void(const char*, const char*, const char*)> func);
    bool Load_Format_Config(const char* default_config, const wchar_t* file_name, std::function<bool(CSimpleIniA&)> func);
protected:
	CExtractor& mExtractor;
protected:
	std::vector<std::wstring> mErrors;		//container for errors encountered during parsing
public:
	CFile_Format_Rules(CFile_Format_Detector& recognizer, CExtractor& extractor, CDateTime_Detector& datetime);

	bool Load();
};
