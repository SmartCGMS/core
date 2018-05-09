/**
 * Portal for online glucose level calculation
 * https://diabetes.zcu.cz/
 *
 * Author: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include <string>

#include "CSVFormat.h"
#include "XMLFormat.h"

#include <xlnt/xlnt.hpp>
#include <ExcelFormat/ExcelFormat.h>

// all known file formats
enum class KnownFileFormats
{
	FORMAT_CSV,
	FORMAT_XLS,
	FORMAT_XLSX,
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

/*
 * XLS file format adapter
 */
class CXls_File : public ISpreadsheet_File
{
	private:
		std::unique_ptr<YExcel::BasicExcel> mFile;
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
