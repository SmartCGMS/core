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

#include <memory>
#include <functional>
#include <assert.h>

#include "file_format_rules.h"

#include "FormatImpl.h"
#include "../../../../common/rtl/FilesystemLib.h"

/*
 * Format adapter class encapsulating file format recognition and interfacing with
 * table-like formats using cellspecs and row/column specifiers
 */
class CFormat_Adapter {
private:
	// format we use
	NStorage_Format mStorage_Fromat = NStorage_Format::unknown;
	std::unique_ptr<IStorage_File> mStorage;
	// original path to file
	filesystem::path mOriginalPath;
	// initializes adapter - creates appropriate structures, ..
	bool Init(const filesystem::path filename, filesystem::path originalFilename = {});

	bool mValid = false;
protected:
	std::string mFormat_Name;
	bool Detect_Format_Layout(const TFormat_Detection_Rules& rules);
protected:
	// converts format pointer to spreadsheet file
	ISpreadsheet_File * ToSpreadsheetFile() const;
	// converts format pointer to hierarchy file
	IHierarchy_File* ToHierarchyFile() const;

public:
	CFormat_Adapter(const TFormat_Detection_Rules&rules, const filesystem::path filename, const filesystem::path originalFilename = {});
	//CFormat_Adapter();
	virtual ~CFormat_Adapter();

	bool Valid() const;
	std::string Format_Name() const;

	// reads from cell using cellspec
	std::string Read(const char* cellSpec) const;
	// reads from cell using coordinates
	std::string Read(int row, int column, int sheetIndex = -1) const;
	// reads from cell using tree position
	std::string Read(TreePosition& position) const;
	// reads floating point value from cell using cellspec
	double Read_Double(const char* cellSpec) const;
	// reads floating point value from cell using coordinates
	double Read_Double(int row, int column, int sheetIndex = -1) const;
	// reads floating point value from cell using tree position
	double Read_Double(TreePosition& position) const;
	// reads date from cell using cellspec
	std::string Read_Date(const char* cellSpec) const;
	// reads date from cell using coordinates
	std::string Read_Date(int row, int column, int sheetIndex = -1) const;
	// reads date from cell using tree position
	std::string Read_Date(TreePosition& position) const;
	// reads time from cell using cellspec
	std::string Read_Time(const char* cellSpec) const;
	// reads time from cell using coordinates
	std::string Read_Time(int row, int column, int sheetIndex = -1) const;
	// reads time from cell using tree position
	std::string Read_Time(TreePosition& position) const;
	// reads datetime string from cell using cellspec
	std::string Read_Datetime(const char* cellSpec) const;
	// reads datetime string from cell using coordinates
	std::string Read_Datetime(int row, int column, int sheetIndex = -1) const;
	// reads datetime string from cell using tree position
	std::string Read_Datetime(TreePosition& position) const;

	// retrieves data organization structure enum value
	NFile_Organization_Structure Get_File_Organization() const;

	// writes to cell using cellspec
	void Write(const char* cellSpec, std::string value);
	// writes to cell using coordinates
	void Write(int row, int column, std::string value, int sheetIndex = -1);
	// writes to cell using tree position
	void Write(TreePosition& position, std::string value);

	bool Is_EOF() const;
	// resets EOF flag
	void Reset_EOF();

	// convert cellspec to coordinates
	static void CellSpec_To_RowCol(const char* cellSpec, int& row, int& col, int& sheetIndex);
	// convert coordinates to cellspec
	static void RowCol_To_CellSpec(int row, int col, std::string& cellSpec);

	// convert cellspec to tree position structure
	static void CellSpec_To_TreePosition(const char* cellSpec, TreePosition& pos);
	// convert tree position structure to cellspec
	static void TreePosition_To_CellSpec(TreePosition& pos, std::string& cellSpec);
};
