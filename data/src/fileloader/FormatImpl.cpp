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

#include "FormatImpl.h"

#include <memory>
#include <cmath>
#ifdef _WIN32
	#include <windows.h>
#endif

#include "../../../../common/utils/string_utils.h"

/** Generic implementations **/

IStorage_File::~IStorage_File()
{
	//
}

bool IStorage_File::Is_EOF() const
{
	return mEOF;
}

void IStorage_File::Reset_EOF()
{
	mEOF = false;
}

ISpreadsheet_File::~ISpreadsheet_File()
{
	//
}

// several read methods falls back to string read, if not supported by format library

std::string ISpreadsheet_File::Read_Date(int row, int col)
{
	return Read(row, col);
}

std::string ISpreadsheet_File::Read_Time(int row, int col)
{
	return Read(row, col);
}

std::string ISpreadsheet_File::Read_Datetime(int row, int col)
{
	return Read(row, col);
}

FileOrganizationStructure ISpreadsheet_File::Get_File_Organization() const
{
	return FileOrganizationStructure::SPREADSHEET;
}

IHierarchy_File::~IHierarchy_File()
{
	//
}

// several read methods falls back to string read, if not supported by format library

std::string IHierarchy_File::Read_Date(TreePosition& position)
{
	return Read(position);
}

std::string IHierarchy_File::Read_Time(TreePosition& position)
{
	return Read(position);
}

std::string IHierarchy_File::Read_Datetime(TreePosition& position)
{
	return Read(position);
}

FileOrganizationStructure IHierarchy_File::Get_File_Organization() const
{
	return FileOrganizationStructure::HIERARCHY;
}

// converts codepage encoded string to unicode; needed due to mixed mode XLS files
std::wstring CodePageToUnicode(int codePage, const char *src)
{
#ifndef _WIN32
	// we do not support codepages on non-Windows machine for now; we would need iconv or similar
	return std::wstring{ src, src + strlen(src) };
#else
	if (!src)
		return L"";

	int srcLen = (int)strlen(src);
	if (!srcLen)
	{
		return L"";
	}

	int requiredSize = MultiByteToWideChar(codePage, 0, src, srcLen, 0, 0);

	if (!requiredSize)
		return L"";

	std::wstring w(requiredSize, L' ');

	int retval = MultiByteToWideChar(codePage, 0, src, srcLen, &w[0], requiredSize);
	if (!retval)
		return L"";

	return w;
#endif
}

// custom function for parsing floating point values
static double stod_custom(std::string& in)
{
	size_t i = 0;
	int expon = 0;
	double ret = 0.0;

	// base
	while (i < in.length() && in[i] >= '0' && in[i] <= '9')
	{
		ret *= 10.0;
		ret += in[i] - '0';
		i++;
	}

	// decimal part
	if (i < in.length() && (in[i] == ',' || in[i] == '.'))
	{
		i++;
		while (i < in.length() && in[i] >= '0' && in[i] <= '9')
		{
			ret *= 10.0;
			ret += in[i] - '0';
			expon--;
			i++;
		}
	}

	// scientific notation
	if (i < in.length() && (in[i] == 'e' || in[i] == 'E'))
	{
		i++;
		bool signflag = true;
		int totExp = 0;
		// parse sign
		if (i < in.length())
		{
			if (in[i] == '-')
			{
				signflag = false;
				i++;
			}
			else if (in[i] == '+')
				i++;
		}

		while (i < in.length() && in[i] >= '0' && in[i] <= '9')
		{
			totExp *= 10;
			totExp += in[i] - '0';
			i++;
		}

		expon += (signflag ? -1 : 1)*totExp;
	}

	// final alignment
	ret *= std::pow(10.0, (double)expon);

	return ret;
}

/** CSV format interface implementation **/

void CCsv_File::Init(filesystem::path &path)
{
	mOriginalPath = path;
	mFile = std::make_unique<CCSV_Format>(path);
}

void CCsv_File::Select_Worksheet(int sheetIndex)
{
	// CSV does not have worksheets, do nothing here
}

std::string CCsv_File::Read(int row, int col)
{
	std::string rd = mFile->Read(row, col);

	if (mFile->Is_UnkCell_Flag())
		mEOF = true;

	return rd;
}

double CCsv_File::Read_Double(int row, int col)
{
	double rd;
	try
	{
		std::string str = mFile->Read(row, col);
		rd = stod_custom(str);
	}
	catch (std::exception&)
	{
		rd = 0.0;
	}

	if (mFile->Is_UnkCell_Flag())
		mEOF = true;

	return rd;
}

void CCsv_File::Write(int row, int col, std::string value)
{
	mFile->Write(row, col, value);
}

void CCsv_File::Finalize()
{
	//
}

#ifndef NO_BUILD_EXCELSUPPORT

// we assume cp1250 in Excel XLS formats for now; TODO: find some recognition pattern
const int assumedCodepage = 1250;

/** XLS format interface implementation **/

void CXls_File::Init(filesystem::path &path)
{
	mSelectedSheetIndex = 0;
	mOriginalPath = path;
	mFile = std::make_unique<ExcelFormat::BasicExcel>();
        const auto converted_path = path.wstring();
        mFile->Load(converted_path.c_str());
}

void CXls_File::Select_Worksheet(int sheetIndex)
{
	mSelectedSheetIndex = sheetIndex;
}

std::string CXls_File::Read(int row, int col)
{
	ExcelFormat::BasicExcelWorksheet* ws = mFile->GetWorksheet(mSelectedSheetIndex);
	if (!ws)
	{
		mEOF = true;
		return std::string("");
	}

	ExcelFormat::BasicExcelCell* cell = ws->Cell(row, col);
	if (!cell || cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED)
	{
		mEOF = true;
		return std::string("");
	}

	// type disambiguation; convert if needed
	switch (cell->Type())
	{
		case ExcelFormat::BasicExcelCell::STRING:
		{
			// unfortunatelly, Excel in XLS format saves several strings in codepage encoding
			std::wstring tmp = CodePageToUnicode(assumedCodepage, cell->GetString());
			return Narrow_WString(tmp);
		}
		case ExcelFormat::BasicExcelCell::WSTRING:
		{
			std::wstring tmp = cell->GetWString();
			return Narrow_WString(tmp);
		}
		case ExcelFormat::BasicExcelCell::DOUBLE:
			return std::to_string(cell->GetDouble());
		case ExcelFormat::BasicExcelCell::INT:
			return std::to_string(cell->GetInteger());
	}

	return std::string("");
}

double CXls_File::Read_Double(int row, int col)
{
	ExcelFormat::BasicExcelWorksheet* ws = mFile->GetWorksheet(mSelectedSheetIndex);
	if (!ws)
	{
		mEOF = true;
		return 0.0;
	}

	ExcelFormat::BasicExcelCell* cell = ws->Cell(row, col);
	if (!cell)
	{
		mEOF = true;
		return 0.0;
	}

	try
	{
		switch (cell->Type())
		{
			case ExcelFormat::BasicExcelCell::STRING:
			{
				std::string str(cell->GetString());
				return stod_custom(str);
			}
			case ExcelFormat::BasicExcelCell::WSTRING:
			{
				std::string str(Narrow_WString(std::wstring(cell->GetWString())));
				return stod_custom(str);
			}
			case ExcelFormat::BasicExcelCell::DOUBLE:
				return cell->GetDouble();
			case ExcelFormat::BasicExcelCell::INT:
				return (double)cell->GetInteger();
		}
	}
	catch (std::exception&)
	{
		// could happen basically only on conversion error
	}

	return 0.0;
}

void CXls_File::Write(int row, int col, std::string value)
{
	ExcelFormat::BasicExcelWorksheet* ws = mFile->GetWorksheet(mSelectedSheetIndex);
	if (!ws)
		return;

	ws->Cell(row, col)->Set(value.c_str());
}

void CXls_File::Finalize()
{
	// we do not need saving in temporary load filters
	//mFile->SaveAs(mOriginalPath.c_str());
}

/** XLSX format interface implementation **/

void CXlsx_File::Init(filesystem::path &path)
{
	mSelectedSheetIndex = 0;
	mOriginalPath = path;
	mFile = std::make_unique<xlnt::workbook>();

	mFile->load(path.string());
}

void CXlsx_File::Select_Worksheet(int sheetIndex)
{
	mSelectedSheetIndex = sheetIndex;
}

std::string CXlsx_File::Read(int row, int col)
{
	try
	{
		xlnt::worksheet ws = mFile->sheet_by_index(mSelectedSheetIndex);

		xlnt::cell const& cl = ws.cell(col + 1, row + 1);

		if (cl.data_type() == xlnt::cell_type::formula_string)
			return cl.value<std::string>();
		else if (cl.data_type() == xlnt::cell_type::number)
			return cl.to_string();
		else if (cl.data_type() == xlnt::cell_type::error)
		{
			mEOF = true;
			return "";
		}
		else
			return "";
	}
	catch (std::exception&)
	{
		mEOF = true;
		return "";
	}
}

double CXlsx_File::Read_Double(int row, int col)
{
	try
	{
		xlnt::worksheet ws = mFile->sheet_by_index(mSelectedSheetIndex);
		return ws.cell(col + 1, row + 1).value<double>();
	}
	catch (std::exception&)
	{
		mEOF = true;
		return 0.0;
	}
}

std::string CXlsx_File::Read_Date(int row, int col)
{
	try
	{
		xlnt::worksheet ws = mFile->sheet_by_index(mSelectedSheetIndex);
		xlnt::cell const& cl = ws.cell(col + 1, row + 1);

		if (cl.data_type() == xlnt::cell_type::error || cl.data_type() == xlnt::cell_type::empty)
		{
			mEOF = true;
			return "";
		}
		else
		{
			xlnt::date dt = cl.value<xlnt::date>();
			xlnt::number_format nf = xlnt::number_format::date_ddmmyyyy();
			return nf.format(dt.to_number(xlnt::calendar::windows_1900), xlnt::calendar::windows_1900);
		}
	}
	catch (std::exception&)
	{
		mEOF = true;
		return "";
	}
}

std::string CXlsx_File::Read_Time(int row, int col)
{
	try
	{
		xlnt::worksheet ws = mFile->sheet_by_index(mSelectedSheetIndex);
		xlnt::cell const& cl = ws.cell(col + 1, row + 1);

		if (cl.data_type() == xlnt::cell_type::error || cl.data_type() == xlnt::cell_type::empty)
		{
			mEOF = true;
			return "";
		}
		else
		{
			xlnt::time tm = cl.value<xlnt::time>();
			xlnt::number_format nf = xlnt::number_format::date_time6();
			return nf.format(tm.to_number(), xlnt::calendar::windows_1900);
		}
	}
	catch (std::exception&)
	{
		mEOF = true;
		return "";
	}
}

std::string CXlsx_File::Read_Datetime(int row, int col)
{
	try
	{
		xlnt::worksheet ws = mFile->sheet_by_index(mSelectedSheetIndex);
		xlnt::cell const& cl = ws.cell(col + 1, row + 1);

		if (cl.data_type() == xlnt::cell_type::error || cl.data_type() == xlnt::cell_type::empty)
		{
			mEOF = true;
			return "";
		}
		else
		{
			xlnt::datetime dtm = cl.value<xlnt::datetime>();
			return dtm.to_string();
		}
	}
	catch (std::exception&)
	{
		mEOF = true;
		return "";
	}
}

void CXlsx_File::Write(int row, int col, std::string value)
{
	try
	{
		xlnt::worksheet ws = mFile->sheet_by_index(mSelectedSheetIndex);
		ws.cell(col + 1, row + 1).value(value);
	}
	catch (std::exception&)
	{
		//
	}
}

void CXlsx_File::Finalize()
{
	// we do not need saving in temporary load filters
	//mFile->save(mOriginalPath);
}

#endif

/** XML format interface implementation **/

void CXml_File::Init(filesystem::path &path)
{
	mOriginalPath = path;
	mFile = std::make_unique<CXML_Format>(path);
}

std::string CXml_File::Read(TreePosition& position)
{
	return mFile->Read(position);
}

double CXml_File::Read_Double(TreePosition& position)
{
	std::string str(Read(position));
	return stod_custom(str);
}

void CXml_File::Write(TreePosition& position, std::string value)
{
	mFile->Write(position, value);
}

void CXml_File::Finalize()
{
	//
}
