/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "FormatImpl.h"

#include <locale>
#include <codecvt>
#include <memory>
#ifdef _WIN32
	#include <windows.h>
#endif

// we assume cp1250 in Excel XLS formats for now; TODO: find some recognition pattern
const int assumedCodepage = 1250;

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

// converts widestring to UTF-8 to allow comparison with patterns, etc.
inline std::string WStringToUTF8(const std::wstring& str)
{
	try
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
		return myconv.to_bytes(str);
	}
	catch (std::exception &)
	{
		return std::string(str.begin(), str.end());
	}
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
	ret *= pow(10.0, (double)expon);

	return ret;
}

/** CSV format interface implementation **/

void CCsv_File::Init(std::wstring &path)
{
	mOriginalPath = path;
	mFile = std::make_unique<CCSV_Format>(path.c_str());
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

/** XLS format interface implementation **/

void CXls_File::Init(std::wstring &path)
{
	mSelectedSheetIndex = 0;
	mOriginalPath = path;
	mFile = std::make_unique<ExcelFormat::BasicExcel>();
	mFile->Load(path.c_str());
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
			return WStringToUTF8(tmp);
		}
		case ExcelFormat::BasicExcelCell::WSTRING:
		{
			std::wstring tmp = cell->GetWString();
			return WStringToUTF8(tmp);
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
				std::string str(WStringToUTF8(std::wstring(cell->GetWString())));
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

void CXlsx_File::Init(std::wstring &path)
{
	mSelectedSheetIndex = 0;
	mOriginalPath = path;
	mFile = std::make_unique<xlnt::workbook>();

	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;
	std::string spath = converterX.to_bytes(path);

	mFile->load(spath);
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

/** XML format interface implementation **/

void CXml_File::Init(std::wstring &path)
{
	mOriginalPath = path;
	mFile = std::make_unique<CXML_Format>(path.c_str());
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
