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
			const std::string rVal = Read(rulePair.first.c_str());

			if (!Contains_Element(rulePair.second, rVal))
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

std::string CFormat_Adapter::Read(const char* cellSpec) const
{
	std::string ret = "";

	switch (Get_File_Organization())
	{
		case NFile_Organization_Structure::SPREADSHEET:
		{
			int row, col, sheet;
			CellSpec_To_RowCol(cellSpec, row, col, sheet);

			ret = Read(row, col, sheet);
			break;
		}
		case NFile_Organization_Structure::HIERARCHY:
		{
			TXML_Position pos;
			CellSpec_To_TreePosition(cellSpec, pos);

			ret = Read(pos);

			if (!pos.Valid())
				ret = "<INVALID TREE POSITION>";
			break;
		}
	}

	return ret;
}

std::string CFormat_Adapter::Read(const TSheet_Position& position) const {
	return Read(position.row, position.column, position.sheetIndex);
}

std::string CFormat_Adapter::Read(int row, int column, int sheetIndex) const
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	return ToSpreadsheetFile()->Read(row, column);
}

std::string CFormat_Adapter::Read(TXML_Position& position) const
{
	return ToHierarchyFile()->Read(position);
}

double CFormat_Adapter::Read_Double(const char* cellSpec) const
{
	int row, col, sheet;
	CellSpec_To_RowCol(cellSpec, row, col, sheet);

	return Read_Double(row, col, sheet);
}

double CFormat_Adapter::Read_Double(const TSheet_Position& position) const {
	return Read_Double(position.row, position.column, position.sheetIndex);
}

double CFormat_Adapter::Read_Double(int row, int column, int sheetIndex) const
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	return ToSpreadsheetFile()->Read_Double(row, column);
}

double CFormat_Adapter::Read_Double(TXML_Position& position) const
{
	return ToHierarchyFile()->Read_Double(position);
}


std::string CFormat_Adapter::Read_Date(const TSheet_Position& position) const {
	return Read_Date(position.row, position.column, position.sheetIndex);
}

std::string CFormat_Adapter::Read_Date(const char* cellSpec) const
{
	int row, col, sheet;
	CellSpec_To_RowCol(cellSpec, row, col, sheet);

	return Read_Date(row, col, sheet);
}

std::string CFormat_Adapter::Read_Date(int row, int column, int sheetIndex) const
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	return ToSpreadsheetFile()->Read_Date(row, column);
}

std::string CFormat_Adapter::Read_Date(TXML_Position& position) const
{
	return ToHierarchyFile()->Read_Date(position);
}

std::string CFormat_Adapter::Read_Time(const char* cellSpec) const
{
	int row, col, sheet;
	CellSpec_To_RowCol(cellSpec, row, col, sheet);

	return Read_Time(row, col, sheet);
}

std::string CFormat_Adapter::Read_Time(int row, int column, int sheetIndex) const
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	return ToSpreadsheetFile()->Read_Time(row, column);
}

std::string CFormat_Adapter::Read_Time(TXML_Position& position) const
{
	return ToHierarchyFile()->Read_Time(position);
}

std::string CFormat_Adapter::Read_Datetime(const char* cellSpec) const
{
	int row, col, sheet;
	CellSpec_To_RowCol(cellSpec, row, col, sheet);

	return Read_Datetime(row, col, sheet);
}

std::string CFormat_Adapter::Read_Datetime(int row, int column, int sheetIndex) const
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	return ToSpreadsheetFile()->Read_Datetime(row, column);
}

std::string CFormat_Adapter::Read_Datetime(TXML_Position& position) const
{
	return ToHierarchyFile()->Read_Datetime(position);
}

void CFormat_Adapter::Write(const char* cellSpec, std::string value)
{
	switch (Get_File_Organization())
	{
		case NFile_Organization_Structure::SPREADSHEET:
		{
			int row, col, sheet;
			CellSpec_To_RowCol(cellSpec, row, col, sheet);

			Write(row, col, value, sheet);
			break;
		}
		case NFile_Organization_Structure::HIERARCHY:
		{
			TXML_Position pos;
			CellSpec_To_TreePosition(cellSpec, pos);

			return Write(pos, value);
		}
	}
}

void CFormat_Adapter::Write(int row, int column, std::string value, int sheetIndex)
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	ToSpreadsheetFile()->Write(row, column, value);
}

void CFormat_Adapter::Write(TXML_Position& position, std::string value)
{
	ToHierarchyFile()->Write(position, value);
}

bool CFormat_Adapter::Is_EOF() const
{
	return mStorage->Is_EOF();
}

void CFormat_Adapter::Reset_EOF()
{
	mStorage->Reset_EOF();
}

NFile_Organization_Structure CFormat_Adapter::Get_File_Organization() const
{
	return mStorage->Get_File_Organization();
}

void CFormat_Adapter::CellSpec_To_RowCol(const char* cellSpec, int& row, int& col, int& sheetIndex)
{
	bool specTypeFlag = false;
	size_t i = 0;

	sheetIndex = 0;

	col = 0;
	while (cellSpec[i] >= 'A' && cellSpec[i] <= 'Z')
	{
		col *= 'Z' - 'A' + 1;
		col += cellSpec[i] - 'A';
		i++;
	}

	// in case of non-Excel cellspec (not beginning with letter), set flag
	if (i == 0)
		specTypeFlag = true;

	row = 0;
	while (cellSpec[i] >= '0' && cellSpec[i] <= '9')
	{
		row *= 10;
		row += cellSpec[i] - '0';
		i++;
	}

	// besides standard Excell cellspec (B8, ..) we recognize also comma-separated cellspec (1,7)
	if (specTypeFlag)
	{
		i++;
		col = 0;
		while (cellSpec[i] >= '0' && cellSpec[i] <= '9')
		{
			col *= 10;
			col += cellSpec[i] - '0';
			i++;
		}
	}
	else
		row--; // decrease row to be universal

	// parse sheet index in case of multisheet workbook (XLS, XLSX)
	if (cellSpec[i] == ':')
	{
		i++;
		sheetIndex = 0;
		while (cellSpec[i] >= '0' && cellSpec[i] <= '9')
		{
			sheetIndex *= 10;
			sheetIndex += cellSpec[i] - '0';
			i++;
		}
	}
}

void CFormat_Adapter::RowCol_To_CellSpec(int row, int col, std::string& cellSpec)
{
	cellSpec = "";

	while (col > 0)
	{
		cellSpec += 'A' + (col % ('Z' - 'A' + 1));
		col /= ('Z' - 'A' + 1);
	}

	cellSpec += std::to_string(row);
}

void CFormat_Adapter::CellSpec_To_TreePosition(const char* cellSpec, TXML_Position& pos)
{
	// Example structure: /rootelement/childelement:0/valueelement:5.Value

	pos.Reset();

	// correct cellspec always contains '/' at the beginning and at least one letter
	if (strlen(cellSpec) <= 1 || cellSpec[0] != '/')
		return;

	size_t i = 1, lastbl = 0, lastcolon = 0;
	size_t len = strlen(cellSpec);
	size_t levelOrdinal = 0;

	for (; i <= len; i++)
	{
		if (i == len || cellSpec[i] == '/' || cellSpec[i] == '.')
		{
			if (lastcolon)
			{
				// invalid - nothing between current character and colon
				if (i - lastcolon <= 1)
				{
					pos.Reset();
					return;
				}

				try
				{
					levelOrdinal = std::stoul(std::string(cellSpec).substr(lastcolon + 1, i - lastcolon - 1));
				}
				catch (...) // invalid - non-numeric ordinal specifier
				{
					pos.Reset();
					return;
				}
			}

			// invalid - two hierarchy splitters without level spec
			if (i - lastbl <= 1)
			{
				pos.Reset();
				return;
			}

			std::string tagName = std::string(cellSpec).substr(lastbl + 1, (lastcolon ? lastcolon : i) - lastbl - 1);

			pos.hierarchy.push_back(TreeLevelSpec(tagName, levelOrdinal));

			lastcolon = 0;
			levelOrdinal = 0;

			if (i != len && cellSpec[i] == '/')
				lastbl = i;
		}

		if (i != len)
		{
			if (cellSpec[i] == ':')
			{
				lastcolon = i;
			}
			else if (cellSpec[i] == '.')
			{
				if (i == len - 1)
				{
					pos.Reset();
					return;
				}

				pos.parameter = std::string(cellSpec).substr(i + 1);
				break;
			}
		}
	}
}

void CFormat_Adapter::TreePosition_To_CellSpec(TXML_Position& pos, std::string& cellSpec)
{
	std::ostringstream os;

	for (size_t i = 0; i < pos.hierarchy.size(); i++)
	{
		os << "/" << pos.hierarchy[i].tagName;

		if (pos.hierarchy[i].position != 0)
			os << ":" << pos.hierarchy[i].position;
	}

	if (!pos.parameter.empty())
		os << "." << pos.parameter;

	cellSpec = os.str();
}
