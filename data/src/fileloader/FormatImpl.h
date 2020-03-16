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

#include <string>
#include <memory>

#include "CSVFormat.h"
#include "XMLFormat.h"

#ifndef NO_BUILD_EXCELSUPPORT
#include <xlnt/xlnt.hpp>

#include <ExcelFormat/ExcelFormat.h>
#endif

// all known file formats
enum class KnownFileFormats
{
	FORMAT_CSV,
#ifndef NO_BUILD_EXCELSUPPORT
	FORMAT_XLS,
	FORMAT_XLSX,
#endif
	FORMAT_XML,
};

enum class FileOrganizationStructure
{
	SPREADSHEET,
	HIERARCHY
};

/*
 * Structure encapsulating position in workbook
 */
struct SheetPosition
{
	// current row
	int row;
	// current column
	int column;
	// sheet index used
	int sheetIndex;

	// default constructor
	SheetPosition() noexcept : sheetIndex(-1) { };
	// copy constructor
	SheetPosition(const SheetPosition& other) : row(other.row), column(other.column), sheetIndex(other.sheetIndex) { };
	// is sheet position valid?
	bool Valid() const { return sheetIndex >= 0; };

	bool operator==(SheetPosition const& other) const
	{
		return ((row == other.row) && (column == other.column) && (sheetIndex == other.sheetIndex));
	}
	bool operator!=(SheetPosition const& other) const
	{
		return !((row == other.row) && (column == other.column) && (sheetIndex == other.sheetIndex));
	}
};

/*
 * Base storage file interface
 */
class IStorage_File
{
	protected:
		// end of file flag
		bool mEOF;
		// original path to file
		std::wstring mOriginalPath;

		public:
		virtual ~IStorage_File();

		// initializes format, loads from file, etc.
		virtual void Init(std::wstring &path) = 0;
		// finalizes working with file
		virtual void Finalize() = 0;
		// retrieves EOF flag
		virtual bool Is_EOF() const;
		// resets EOF flag
		virtual void Reset_EOF();

		// retrieves file data organization structure
		virtual FileOrganizationStructure Get_File_Organization() const = 0;
};

/*
 * Spreadsheet format adapter interface; encapsulates all needed operations on table file types
 */
class ISpreadsheet_File : public IStorage_File
{
	private:
		//

	protected:
		//

	public:
		// virtual destructor due to need of calling derived ones
		virtual ~ISpreadsheet_File();

		// initializes format, loads from file, etc.
		virtual void Init(std::wstring &path) = 0;
		// selects worksheet to work with
		virtual void Select_Worksheet(int sheetIndex) = 0;
		// reads cell contents (string)
		virtual std::string Read(int row, int col) = 0;
		// reads cell contents (double)
		virtual double Read_Double(int row, int col) = 0;
		// reads cell contents (date string)
		virtual std::string Read_Date(int row, int col);
		// reads cell contents (time string)
		virtual std::string Read_Time(int row, int col);
		// reads cell contents (datetime string)
		virtual std::string Read_Datetime(int row, int col);
		// writes cell contents
		virtual void Write(int row, int col, std::string value) = 0;
		// finalizes working with file
		virtual void Finalize() = 0;

		// retrieves file data organization structure
		virtual FileOrganizationStructure Get_File_Organization() const;
};

/*
 * Hierarchy format adapter interface; encapsulates all needed operations on tree hierarchy file types
 */
class IHierarchy_File : public IStorage_File
{
	private:
		//

	protected:
		// end of file flag
		bool mEOF;

	public:
		// virtual destructor due to need of calling derived ones
		virtual ~IHierarchy_File();

		// initializes format, loads from file, etc.
		virtual void Init(std::wstring &path) = 0;
		// reads cell contents (string)
		virtual std::string Read(TreePosition& position) = 0;
		// reads cell contents (double)
		virtual double Read_Double(TreePosition& position) = 0;
		// reads cell contents (date string)
		virtual std::string Read_Date(TreePosition& position);
		// reads cell contents (time string)
		virtual std::string Read_Time(TreePosition& position);
		// reads cell contents (datetime string)
		virtual std::string Read_Datetime(TreePosition& position);
		// writes cell contents
		virtual void Write(TreePosition& position, std::string value) = 0;
		// finalizes working with file
		virtual void Finalize() = 0;

		// retrieves file data organization structure
		FileOrganizationStructure Get_File_Organization() const;
};

/*
 * CSV file format adapter
 */
class CCsv_File : public ISpreadsheet_File
{
	private:
		std::unique_ptr<CCSV_Format> mFile;

	public:
		virtual void Init(std::wstring &path);
		virtual void Select_Worksheet(int sheetIndex);
		virtual std::string Read(int row, int col);
		virtual double Read_Double(int row, int col);
		virtual void Write(int row, int col, std::string value);
		virtual void Finalize();
};

#ifndef NO_BUILD_EXCELSUPPORT

/*
 * XLS file format adapter
 */
class CXls_File : public ISpreadsheet_File
{
	private:
		std::unique_ptr<ExcelFormat::BasicExcel> mFile;
		int mSelectedSheetIndex;

	public:
		virtual void Init(std::wstring &path);
		virtual void Select_Worksheet(int sheetIndex);
		virtual std::string Read(int row, int col);
		virtual double Read_Double(int row, int col);
		virtual void Write(int row, int col, std::string value);
		virtual void Finalize();
};

/*
 * XLSX file format adapter
 */
class CXlsx_File : public ISpreadsheet_File
{
	private:
		std::unique_ptr<xlnt::workbook> mFile;
		int mSelectedSheetIndex;

	public:
		virtual void Init(std::wstring &path);
		virtual void Select_Worksheet(int sheetIndex);
		virtual std::string Read(int row, int col);
		virtual double Read_Double(int row, int col);
		virtual std::string Read_Date(int row, int col);
		virtual std::string Read_Time(int row, int col);
		virtual std::string Read_Datetime(int row, int col);
		virtual void Write(int row, int col, std::string value);
		virtual void Finalize();
};

#endif

/*
 * XML format adapter
 */
class CXml_File : public IHierarchy_File
{
	private:
		std::unique_ptr<CXML_Format> mFile;

	public:
		virtual void Init(std::wstring &path);
		virtual std::string Read(TreePosition& position);
		virtual double Read_Double(TreePosition& position);
		virtual void Write(TreePosition& position, std::string value);
		virtual void Finalize();
};
