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

#include <string>
#include <memory>
#include <optional>

#include "../../../../common/rtl/guid.h"
#include "../../../../common/rtl/FilesystemLib.h"

#include "CSVFormat.h"
#include "XMLFormat.h"
#include "Misc.h"

#ifndef NO_BUILD_EXCELSUPPORT
#include <xlnt/xlnt.hpp>

#include <ExcelFormat/ExcelFormat.h>
#endif

// all known file formats
enum class NStorage_Format
{
	csv,
#ifndef NO_BUILD_EXCELSUPPORT
	xls,
	xlsx,
#endif
	xml,
	unknown
};

enum class NFile_Organization_Structure
{
	SPREADSHEET,
	HIERARCHY
};

/*
 * Structure encapsulating position in workbook
 */
struct TSheet_Position
{
	// current row
	int row = 0;
	// current column
	int column = 0;
	// sheet index used
	int sheetIndex = -1;

	// default constructor
	TSheet_Position() noexcept : sheetIndex(-1) { };
	// copy constructor
	TSheet_Position(const TSheet_Position& other) : row(other.row), column(other.column), sheetIndex(other.sheetIndex) { };
	
	TSheet_Position(int r, int c, int s) : row(r), column(c), sheetIndex(s) {};

	bool operator==(TSheet_Position const& other) const
	{
		return ((row == other.row) && (column == other.column) && (sheetIndex == other.sheetIndex));
	}
	bool operator!=(TSheet_Position const& other) const
	{
		return !((row == other.row) && (column == other.column) && (sheetIndex == other.sheetIndex));
	}

	// is sheet position valid?
	bool Valid() const 	{
		return (row >= 0) && (column >= 0) && (sheetIndex >= 0);
	};

	void Reset() { 
		row = 0;
		column = 0;
		sheetIndex = -1;
	};
	
	void Forward() {
		row++;
	}
};

//helper functions
TSheet_Position CellSpec_To_RowCol(const std::string& cellSpec);
TXML_Position CellSpec_To_TreePosition(const std::string& cellSpec);


/*
 * Base storage file interface
 */
class IStorage_File
{
	protected:
		// end of file flag
		bool mEOF = false;
		// original path to file
		filesystem::path mOriginalPath;
		// cache mode
		NCache_Mode mCache_Mode = NCache_Mode::Cached;

	public:
		virtual ~IStorage_File() = default;

		// initializes format, loads from file, etc.
		virtual bool Init(filesystem::path &path) = 0;
		// finalizes working with file
		virtual void Finalize() = 0;
		// retrieves EOF flag
		virtual bool Is_EOF() const;
		// resets EOF flag
		virtual void Reset_EOF();
		// sets the cache mode for given instance
		virtual void Set_Cache_Mode(NCache_Mode mode) { mCache_Mode = mode; };

		// retrieves file data organization structure
		virtual NFile_Organization_Structure Get_File_Organization() const = 0;

		virtual std::optional<std::string> Read(const std::string& position) = 0;
		virtual std::optional<std::string> Read(const TSheet_Position& position) { return std::nullopt; }
		virtual std::optional<std::string> Read(const TXML_Position& position) { return std::nullopt; }			//read may modify position to allow sanitization

		virtual bool Condition_Match(const std::string& position) = 0;
		virtual bool Condition_Match(const TSheet_Position& position) { return true; }
		virtual bool Condition_Match(const TXML_Position& position) { return true; }

		virtual bool Position_Valid(const std::string& position) = 0;
		virtual bool Position_Valid(const TSheet_Position& position) { return true; }
		virtual bool Position_Valid(const TXML_Position& position) { return true; }
};

/*
 * Spreadsheet format adapter interface; encapsulates all needed operations on table file types
 */
class ISpreadsheet_File : public virtual IStorage_File
{
	private:
		//

	protected:
		//

	public:
		// virtual destructor due to need of calling derived ones
		virtual ~ISpreadsheet_File() = default;
		
		// writes cell contents
		virtual void Write(int row, int col, int sheetIndex, const std::string& value) = 0;

		// retrieves file data organization structure
		virtual NFile_Organization_Structure Get_File_Organization() const override;

		using IStorage_File::Read;
		using IStorage_File::Condition_Match;
		using IStorage_File::Position_Valid;

		virtual std::optional<std::string> Read(const std::string& position) override final {
			const TSheet_Position pos = CellSpec_To_RowCol(position);
			return Read(pos);
		}

		virtual bool Condition_Match(const std::string& position) override final {
			const TSheet_Position pos = CellSpec_To_RowCol(position);
			return Condition_Match(pos);
		}

		virtual bool Position_Valid(const std::string& position) override final {
			const TSheet_Position pos = CellSpec_To_RowCol(position);
			return Position_Valid(pos);
		}
};

/*
 * Hierarchy format adapter interface; encapsulates all needed operations on tree hierarchy file types
 */
class IHierarchy_File : public virtual IStorage_File
{
	private:
		//

	protected:
		// end of file flag
		bool mEOF = false;

	public:
		// virtual destructor due to need of calling derived ones
		virtual ~IHierarchy_File() = default;

		// writes cell contents
		virtual void Write(TXML_Position& position, const std::string &value) = 0;

		// retrieves file data organization structure
		NFile_Organization_Structure Get_File_Organization() const override;

		using IStorage_File::Condition_Match;
		virtual bool Condition_Match(const std::string& position) override final {
			const TXML_Position pos = CellSpec_To_TreePosition(position);
			return Condition_Match(pos);
		}

		using IStorage_File::Position_Valid;
		virtual bool Position_Valid(const std::string& position) override final {
			const TXML_Position pos = CellSpec_To_TreePosition(position);
			return Position_Valid(pos);
		}
};

/*
 * CSV file format adapter
 */
class CCsv_File : public virtual ISpreadsheet_File
{
	private:
		std::unique_ptr<CCSV_Format> mFile;
	public:
		virtual ~CCsv_File() = default;

		virtual bool Init(filesystem::path &path) override;
		virtual void Write(int row, int col, int sheetIndex, const std::string& value) override;
		virtual void Finalize() override;
		void Set_Cache_Mode(NCache_Mode mode) override {
			ISpreadsheet_File::Set_Cache_Mode(mode);

			if (mFile)
				mFile->Set_Cache_Mode(mode);
		}

		// indicate the intent to override Read from IStorage_File (grandparent) and not "hide" the one in ISpreadsheet_File (parent)
		using IStorage_File::Read;
		virtual std::optional<std::string> Read(const TSheet_Position& position) override final;

		using IStorage_File::Condition_Match;
		virtual bool Condition_Match(const TSheet_Position& position) override final { return true; } // TODO
};

#ifndef NO_BUILD_EXCELSUPPORT

/*
 * XLS file format adapter
 */
class CXls_File : public virtual ISpreadsheet_File
{
	private:
		std::unique_ptr<ExcelFormat::BasicExcel> mFile;
	public:
		virtual ~CXls_File() = default;

		virtual bool Init(filesystem::path &path) override;
		virtual void Write(int row, int col, int sheetIndex, const std::string& value) override;
		virtual void Finalize() override;

		using IStorage_File::Read;
		virtual std::optional<std::string> Read(const TSheet_Position& position) override final;

		using IStorage_File::Condition_Match;
		virtual bool Condition_Match(const TSheet_Position& position) override final { return true; } // TODO
};

/*
 * XLSX file format adapter
 */
class CXlsx_File : public virtual ISpreadsheet_File
{
	private:
		std::unique_ptr<xlnt::workbook> mFile;
		std::optional<std::string> Read_Datetime(const TSheet_Position &position);
	public:
		virtual ~CXlsx_File() = default;

		virtual bool Init(filesystem::path &path) override;
		virtual void Write(int row, int col, int sheetIndex, const std::string& value) override;
		virtual void Finalize() override;

		using IStorage_File::Read;
		virtual std::optional<std::string> Read(const TSheet_Position& position) override final;

		using IStorage_File::Condition_Match;
		virtual bool Condition_Match(const TSheet_Position& position) override final { return true; } // TODO
};

#endif

/*
 * XML format adapter
 */
class CXml_File : public virtual IHierarchy_File
{
	private:
		std::unique_ptr<CXML_Format> mFile;

	public:
		virtual ~CXml_File() = default;

		virtual bool Init(filesystem::path &path) override;
		virtual void Write(TXML_Position& position, const std::string &value) override final;
		virtual void Finalize() override;


		virtual std::optional<std::string> Read(const std::string& position) override final;
		virtual std::optional<std::string> Read(const TXML_Position& position) override final;	//read may modify position

		using IStorage_File::Condition_Match;
		virtual bool Condition_Match(const TXML_Position& position) override final;
		
		using IStorage_File::Position_Valid;
		virtual bool Position_Valid(const TXML_Position& position) override final;
};
