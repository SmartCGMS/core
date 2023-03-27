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

#include "FormatAdapter.h"

#include "CSVFormat.h"
#include "Misc.h"

#include <sstream>
#include <algorithm>
#include <cstring>

/*CFormat_Adapter::CFormat_Adapter()
{
	//
}
*/
CFormat_Adapter::CFormat_Adapter(const TFormat_Detection_Rules& rules, const filesystem::path filename, const filesystem::path originalFilename) {
	mValid = Init(filename, originalFilename);
	if (mValid) {
		mValid = Detect_Format_Layout(rules);
	}
}

CFormat_Adapter::~CFormat_Adapter()
{
	if (mStorage)
		mStorage->Finalize();
}


bool CFormat_Adapter::Valid() const {
	return mValid;
}

std::string CFormat_Adapter::Format_Name() const {
	return mFormat_Name;
}

bool CFormat_Adapter::Detect_Format_Layout(const TFormat_Detection_Rules& layout_rules) {

	bool format_found = false;

	auto match = [&](std::string data_format)->bool {

		// find format to be matched
		auto itr = layout_rules.find(data_format);
		if (itr == layout_rules.end())
			return false;

		// try all rules, all rules need to be matched
		for (auto const& rulePair : itr->second)
		{
			const auto rVal = Read<std::string>(rulePair.first);
			if (!rVal.has_value())
				return false;			 

			if (!Contains_Element(rulePair.second, rVal.value()))
				return false;
		}

		return true;
	};

	// try to recognize data format
	for (auto const& fpair : layout_rules) {
		if (match(fpair.first)) {
			mFormat_Name = fpair.first;
			format_found = true;
			break;
		}
	}

	return format_found;
}

bool CFormat_Adapter::Init(const filesystem::path filename, filesystem::path originalFilename) {
	if (originalFilename.empty())
		originalFilename = filename;

	// at first, try to recognize file format from extension

	mStorage_Fromat = NStorage_Format::unknown;
	std::wstring path = originalFilename.wstring();
	if (path.length() < 4) {		
		return false;
	}

	// find dot
	size_t dotpos = path.find_last_of('.');
	if (dotpos == std::wstring::npos) {		
		return false;
	}
	// extract extension
	std::wstring ext = path.substr(dotpos + 1);
	// convert to lowercase
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// extract format name
	if (ext == L"csv" || ext == L"txt")
		mStorage_Fromat = NStorage_Format::csv;
#ifndef NO_BUILD_EXCELSUPPORT
	else if (ext == L"xls")
		mStorage_Fromat = NStorage_Format::xls;
	else if (ext == L"xlsx")
		mStorage_Fromat = NStorage_Format::xlsx;
#endif
	else if (ext == L"xml")
		mStorage_Fromat = NStorage_Format::xml;
	else {
		return false;
	}

	mOriginalPath = filename;

	// create appropriate format adapter
	switch (mStorage_Fromat)
	{
		case NStorage_Format::csv:
		{
			mStorage = std::make_unique<CCsv_File>();
			break;
		}
#ifndef NO_BUILD_EXCELSUPPORT
		case NStorage_Format::xls:
		{
			mStorage = std::make_unique<CXls_File>();
			break;
		}
		case NStorage_Format::xlsx:
		{
			mStorage = std::make_unique<CXlsx_File>();
			break;
		}
#endif
		case NStorage_Format::xml:
		{
			mStorage = std::make_unique<CXml_File>();
			break;
		}
		default:
			assert("Unsupported format supplied as argument to CFormat_Adapter constructor" && false);
			return false;
	}

	return mStorage->Init(mOriginalPath);
}

ISpreadsheet_File* CFormat_Adapter::ToSpreadsheetFile() const
{
	return dynamic_cast<ISpreadsheet_File*>(mStorage.get());
}

IHierarchy_File* CFormat_Adapter::ToHierarchyFile() const
{
	return dynamic_cast<IHierarchy_File*>(mStorage.get());
}

bool CFormat_Adapter::Is_EOF() const
{
	return mStorage->Is_EOF();
}

void CFormat_Adapter::Reset_EOF()
{
	mStorage->Reset_EOF();
}

NFile_Organization_Structure CFormat_Adapter::Get_File_Organization() const {
	return mStorage->Get_File_Organization();
}
