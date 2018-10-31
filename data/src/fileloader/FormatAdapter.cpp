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
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "FormatAdapter.h"
#include "CSVFormat.h"

#include <sstream>
#include <algorithm>

CFormat_Adapter::CFormat_Adapter()
{
	//
}

CFormat_Adapter::CFormat_Adapter(const wchar_t* filename, const wchar_t* originalFilename)
{
	Init(filename, originalFilename);
}

CFormat_Adapter::~CFormat_Adapter()
{
	if (mFormat)
		mFormat->Finalize();
}

void CFormat_Adapter::Init(const wchar_t* filename, const wchar_t* originalFilename)
{
	mError = 0;
	if (originalFilename == nullptr)
		originalFilename = filename;

	// at first, try to recognize file format from extension

	KnownFileFormats format;
	std::wstring path(originalFilename);
	if (path.length() < 4)
	{
		mError = 1;
		return;
	}

	// find dot
	size_t dotpos = path.find_last_of('.');
	if (dotpos == std::wstring::npos)
	{
		mError = 1;
		return;
	}
	// extract extension
	std::wstring ext = path.substr(dotpos + 1);
	// convert to lowercase
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// extract format name
	if (ext == L"csv" || ext == L"txt")
		format = KnownFileFormats::FORMAT_CSV;
	else if (ext == L"xls")
		format = KnownFileFormats::FORMAT_XLS;
	else if (ext == L"xlsx")
		format = KnownFileFormats::FORMAT_XLSX;
	else if (ext == L"xml")
		format = KnownFileFormats::FORMAT_XML;
	else
	{
		mError = 2;
		return;
	}

	mOriginalPath = filename;

	// create appropriate format adapter
	switch (format)
	{
		case KnownFileFormats::FORMAT_CSV:
		{
			mFormat = std::make_unique<CCsv_File>();
			break;
		}
		case KnownFileFormats::FORMAT_XLS:
		{
			mFormat = std::make_unique<CXls_File>();
			break;
		}
		case KnownFileFormats::FORMAT_XLSX:
		{
			mFormat = std::make_unique<CXlsx_File>();
			break;
		}
		case KnownFileFormats::FORMAT_XML:
		{
			mFormat = std::make_unique<CXml_File>();
			break;
		}
		default:
			assert("Unsupported format supplied as argument to CFormat_Adapter constructor" && false);
			return;
	}

	mFormat->Init(mOriginalPath);
}

ISpreadsheet_File* CFormat_Adapter::ToSpreadsheetFile() const
{
	return dynamic_cast<ISpreadsheet_File*>(mFormat.get());
}

IHierarchy_File* CFormat_Adapter::ToHierarchyFile() const
{
	return dynamic_cast<IHierarchy_File*>(mFormat.get());
}

std::string CFormat_Adapter::Read(const char* cellSpec) const
{
	std::string ret = "";

	switch (Get_File_Organization())
	{
		case FileOrganizationStructure::SPREADSHEET:
		{
			int row, col, sheet;
			CellSpec_To_RowCol(cellSpec, row, col, sheet);

			ret = Read(row, col, sheet);
			break;
		}
		case FileOrganizationStructure::HIERARCHY:
		{
			TreePosition pos;
			CellSpec_To_TreePosition(cellSpec, pos);

			ret = Read(pos);

			if (!pos.Valid())
				ret = "<INVALID TREE POSITION>";
			break;
		}
	}

	return ret;
}

std::string CFormat_Adapter::Read(int row, int column, int sheetIndex) const
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	return ToSpreadsheetFile()->Read(row, column);
}

std::string CFormat_Adapter::Read(TreePosition& position) const
{
	return ToHierarchyFile()->Read(position);
}

double CFormat_Adapter::Read_Double(const char* cellSpec) const
{
	int row, col, sheet;
	CellSpec_To_RowCol(cellSpec, row, col, sheet);

	return Read_Double(row, col, sheet);
}

double CFormat_Adapter::Read_Double(int row, int column, int sheetIndex) const
{
	if (sheetIndex >= 0)
		ToSpreadsheetFile()->Select_Worksheet(sheetIndex);

	return ToSpreadsheetFile()->Read_Double(row, column);
}

double CFormat_Adapter::Read_Double(TreePosition& position) const
{
	return ToHierarchyFile()->Read_Double(position);
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

std::string CFormat_Adapter::Read_Date(TreePosition& position) const
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

std::string CFormat_Adapter::Read_Time(TreePosition& position) const
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

std::string CFormat_Adapter::Read_Datetime(TreePosition& position) const
{
	return ToHierarchyFile()->Read_Datetime(position);
}

void CFormat_Adapter::Write(const char* cellSpec, std::string value)
{
	switch (Get_File_Organization())
	{
		case FileOrganizationStructure::SPREADSHEET:
		{
			int row, col, sheet;
			CellSpec_To_RowCol(cellSpec, row, col, sheet);

			Write(row, col, value, sheet);
			break;
		}
		case FileOrganizationStructure::HIERARCHY:
		{
			TreePosition pos;
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

void CFormat_Adapter::Write(TreePosition& position, std::string value)
{
	ToHierarchyFile()->Write(position, value);
}

int CFormat_Adapter::Get_Error() const
{
	return mError;
}

void CFormat_Adapter::Clear_Error()
{
	mError = 0;
}

bool CFormat_Adapter::Is_EOF() const
{
	return mFormat->Is_EOF();
}

void CFormat_Adapter::Reset_EOF()
{
	mFormat->Reset_EOF();
}

FileOrganizationStructure CFormat_Adapter::Get_File_Organization() const
{
	return mFormat->Get_File_Organization();
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

void CFormat_Adapter::CellSpec_To_TreePosition(const char* cellSpec, TreePosition& pos)
{
	// Example structure: /rootelement/childelement:0/valueelement:5.Value

	pos.Clear();

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
					pos.Clear();
					return;
				}

				try
				{
					levelOrdinal = std::stoul(std::string(cellSpec).substr(lastcolon + 1, i - lastcolon - 1));
				}
				catch (...) // invalid - non-numeric ordinal specifier
				{
					pos.Clear();
					return;
				}
			}

			// invalid - two hierarchy splitters without level spec
			if (i - lastbl <= 1)
			{
				pos.Clear();
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
					pos.Clear();
					return;
				}

				pos.parameter = std::string(cellSpec).substr(i + 1);
				break;
			}
		}
	}
}

void CFormat_Adapter::TreePosition_To_CellSpec(TreePosition& pos, std::string& cellSpec)
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
