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

#include "FormatImpl.h"

#include <memory>
#include <cmath>
#include <sstream>

#ifdef _WIN32
	#include <windows.h>
#endif

#include <scgms/utils/string_utils.h>

//helper functions
TSheet_Position CellSpec_To_RowCol(const std::string &cellSpec) {
	int row = 0, col =0, sheetIndex = 0;

	bool specTypeFlag = false;
	size_t i = 0;

	while (cellSpec[i] >= 'A' && cellSpec[i] <= 'Z') {
		col *= 'Z' - 'A' + 1;
		col += cellSpec[i] - 'A';
		i++;
	}

	// in case of non-Excel cellspec (not beginning with letter), set flag
	if (i == 0) {
		specTypeFlag = true;
	}
	
	while (cellSpec[i] >= '0' && cellSpec[i] <= '9') {
		row *= 10;
		row += cellSpec[i] - '0';
		i++;
	}

	// besides standard Excell cellspec (B8, ..) we recognize also comma-separated cellspec (1,7)
	if (specTypeFlag) {
		i++;
		col = 0;
		while (cellSpec[i] >= '0' && cellSpec[i] <= '9') {
			col *= 10;
			col += cellSpec[i] - '0';
			i++;
		}
	}
	else {
		row--; // decrease row to be universal
	}

	// parse sheet index in case of multisheet workbook (XLS, XLSX)
	if (cellSpec[i] == ':') {
		i++;
		sheetIndex = 0;
		while (cellSpec[i] >= '0' && cellSpec[i] <= '9') {
			sheetIndex *= 10;
			sheetIndex += cellSpec[i] - '0';
			i++;
		}
	}
	
	return TSheet_Position{ row, col, sheetIndex };
}


void RowCol_To_CellSpec(int row, int col, std::string& cellSpec) {
	cellSpec = "";

	while (col > 0) {
		cellSpec += 'A' + (col % ('Z' - 'A' + 1));
		col /= ('Z' - 'A' + 1);
	}

	cellSpec += std::to_string(row);
}

TXML_Position CellSpec_To_TreePosition(const std::string &cellSpec) {
	// Example of conditional structure: /rootelement/childelement.Value&Type:SomeValue
	// Example of conditional structure: /rootelement/childelement.Value&>ChildElementTag.Type:SomeValue
	TXML_Position pos;
	pos.Reset();

	// correct cellspec always contains '/' at the beginning and at least one letter
	if (cellSpec.size() <= 1 || cellSpec[0] != '/') {
		return pos;
	}

	enum class _NState {
		Spec_Parent_Tree,
		Spec_Parameter,
		Condition,
		Cond_Parent_Tree,
		Cond_Parameter,
		Cond_Value,
	};

	_NState curState = _NState::Spec_Parent_Tree;
	TXML_Position::TParam_Cond tmpCond;

	std::ostringstream buffer;
	auto flushBuffer = [&buffer]() -> std::string {
		std::string ret = buffer.str();

		buffer.str(std::string());
		buffer.clear();

		return ret;
	};

	for (size_t i = 1; i <= cellSpec.size(); i++) { // we start from 1, as we already know that the 0th position is '/' and made the "transition" to the correct state (code simplification)
		// virtually add 0-terminator to indicate end of sequence and avoid code duplication
		char c = (i < cellSpec.size()) ? cellSpec[i] : '\0';

		switch (curState)
		{
			case _NState::Spec_Parent_Tree:
			{
				if (c == '/' || c == '.') {
					pos.hierarchy.push_back(TreeLevelSpec{ flushBuffer() });

					if (c == '.') {
						curState = _NState::Spec_Parameter;
					}
				}
				else if (c != '\0') {
					buffer << c;
				}

				break;
			}
			case _NState::Spec_Parameter:
			{
				if (c == '&' || c == '\0') {
					pos.parameter = flushBuffer();
					curState = _NState::Condition;
				}
				else if (c != '\0') {
					buffer << c;
				}
				break;
			}
			case _NState::Condition:
			{
				if (c == '>') {
					curState = _NState::Cond_Parent_Tree;
				}
				else  if (c != '\0') {
					buffer << c;
					curState = _NState::Cond_Parameter;
				}
				break;
			}
			case _NState::Cond_Parent_Tree:
			{
				if (c == '/' || c == '.') {
					tmpCond.subtreeSpec.push_back(TreeLevelSpec{ flushBuffer() });

					if (c == '.') {
						curState = _NState::Cond_Parameter;
					}
				}
				else if (c != '\0') {
					buffer << c;
				}
				break;
			}
			case _NState::Cond_Parameter:
			{
				if (c == ':') {
					tmpCond.parameterName = flushBuffer();
					curState = _NState::Cond_Value;
				}
				else if (c != '\0') {
					buffer << c;
				}
				break;
			}
			case _NState::Cond_Value:
			{
				if (c == '&' || c == '\0') {
					tmpCond.conditionalValue = flushBuffer();

					pos.conditions.push_back(tmpCond);
					tmpCond.Reset();

					curState = _NState::Condition;
				}
				else if (c != '\0') {
					buffer << c;
				}
				break;
			}
		}
	}

	return pos;
}

void TreePosition_To_CellSpec(TXML_Position& pos, std::string& cellSpec)
{
	std::ostringstream os;

	for (size_t i = 0; i < pos.hierarchy.size(); i++) {
		os << "/" << pos.hierarchy[i].tagName;

		if (pos.hierarchy[i].position != 0) {
			os << ":" << pos.hierarchy[i].position;
		}
	}

	if (!pos.parameter.empty()) {
		os << "." << pos.parameter;
	}

	cellSpec = os.str();
}


/** Generic implementations **/

bool IStorage_File::Is_EOF() const {
	return mEOF;
}

void IStorage_File::Reset_EOF() {
	mEOF = false;
}


NFile_Organization_Structure ISpreadsheet_File::Get_File_Organization() const {
	return NFile_Organization_Structure::SPREADSHEET;
}

NFile_Organization_Structure IHierarchy_File::Get_File_Organization() const {
	return NFile_Organization_Structure::HIERARCHY;
}

// converts codepage encoded string to unicode; needed due to mixed mode XLS files
std::wstring CodePageToUnicode(int codePage, const char* src) {
#ifndef _WIN32
	// we do not support codepages on non-Windows machine for now; we would need iconv or similar
	return std::wstring{ src, src + strlen(src) };
#else
	if (!src) {
		return L"";
	}

	int srcLen = (int)strlen(src);
	if (!srcLen) {
		return L"";
	}

	int requiredSize = MultiByteToWideChar(codePage, 0, src, srcLen, 0, 0);

	if (!requiredSize) {
		return L"";
	}

	std::wstring w(requiredSize, L' ');

	int retval = MultiByteToWideChar(codePage, 0, src, srcLen, &w[0], requiredSize);
	if (!retval) {
		return L"";
	}

	return w;
#endif
}


/** CSV format interface implementation **/

bool CCsv_File::Init(filesystem::path &path) {
	mOriginalPath = path;
	mFile = std::make_unique<CCSV_Format>(path);
	return mFile.operator bool();
}


std::optional<std::string> CCsv_File::Read(const TSheet_Position& position) {
	const auto rd = mFile->Read(position.row, position.column);

	if (mFile->Is_UnkCell_Flag()) {
		mEOF = true;
	}

	return rd;
}

void CCsv_File::Write(int row, int col, int sheetIndex, const std::string& value) {
	mFile->Write(row, col, value);
}

void CCsv_File::Finalize() {
	//
}

#ifndef NO_BUILD_EXCELSUPPORT

// we assume cp1250 in Excel XLS formats for now; TODO: find some recognition pattern
const int assumedCodepage = 1250;

/** XLS format interface implementation **/

bool CXls_File::Init(filesystem::path &path) {
	mOriginalPath = path;
	mFile = std::make_unique<ExcelFormat::BasicExcel>();
	bool result = mFile.operator bool();
	if (result) {
		const auto converted_path = path.wstring();
		result = mFile->Load(converted_path.c_str());
	}	
	
	return result;
}

std::optional<std::string> CXls_File::Read(const TSheet_Position& position) {
	ExcelFormat::BasicExcelWorksheet* ws = mFile->GetWorksheet(position.sheetIndex);
	if (!ws) {
		mEOF = true;
		return std::nullopt;
	}

	ExcelFormat::BasicExcelCell* cell = ws->Cell(position.row, position.column);
	if (!cell || cell->Type() == ExcelFormat::BasicExcelCell::UNDEFINED) {
		mEOF = true;
		return std::nullopt;
	}

	// type disambiguation; convert if needed
	switch (cell->Type()) {
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

	return std::nullopt;
}

void CXls_File::Write(int row, int col, int sheetIndex, const std::string& value) {
	ExcelFormat::BasicExcelWorksheet* ws = mFile->GetWorksheet(sheetIndex);
	if (!ws) {
		return;
	}

	ws->Cell(row, col)->Set(value.c_str());
}

void CXls_File::Finalize() {
	// we do not need saving in temporary load filters
	//mFile->SaveAs(mOriginalPath.c_str());
}

/** XLSX format interface implementation **/

bool CXlsx_File::Init(filesystem::path &path) {
	mOriginalPath = path;
	mFile = std::make_unique<xlnt::workbook>();

	bool result = mFile.operator bool();
	if (result) {
		mFile->load(path.string());
	}

	return result;
}

std::optional<std::string> CXlsx_File::Read(const TSheet_Position& position) {
	try {
		xlnt::worksheet ws = mFile->sheet_by_index(static_cast<size_t>(position.sheetIndex));

		xlnt::cell const& cl = ws.cell(position.column + 1, position.row + 1);

		if (cl.data_type() == xlnt::cell_type::formula_string) {
			return cl.value<std::string>();
		}
		else if (cl.data_type() == xlnt::cell_type::number) {
			return cl.to_string();
		}
		else if (cl.data_type() == xlnt::cell_type::date) {
			return Read_Datetime(position);
		}
		else if (cl.data_type() == xlnt::cell_type::error) {
			mEOF = true;
			return std::nullopt;
		}
		else {
			return std::nullopt;
		}
	}
	catch (std::exception&) {
		mEOF = true;
		return std::nullopt;
	}
}

/*
* 
* legacy methods, which we hope we won't need in future, but who knows... so, we rather keep them

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
*/

std::optional<std::string> CXlsx_File::Read_Datetime(const TSheet_Position& position) {
	try {
		xlnt::worksheet ws = mFile->sheet_by_index(position.sheetIndex);
		xlnt::cell const& cl = ws.cell(position.column + 1, position.row + 1);

		if (cl.data_type() == xlnt::cell_type::error || cl.data_type() == xlnt::cell_type::empty) {
			mEOF = true;
			return std::nullopt;
		}
		else {
			xlnt::datetime dtm = cl.value<xlnt::datetime>();
			return dtm.to_string();
		}
	}
	catch (std::exception&) {
		mEOF = true;
		return std::nullopt;
	}
}

void CXlsx_File::Write(int row, int col, int sheetIndex, const std::string &value) {
	try {
		xlnt::worksheet ws = mFile->sheet_by_index(sheetIndex);
		ws.cell(col + 1, row + 1).value(value);
	}
	catch (std::exception&) {
		//
	}
}

void CXlsx_File::Finalize() {
	// we do not need saving in temporary load filters
	//mFile->save(mOriginalPath);
}

#endif

/** XML format interface implementation **/

bool CXml_File::Init(filesystem::path &path) {
	mOriginalPath = path;
	mFile = std::make_unique<CXML_Format>(path);
	return mFile.operator bool();
}

std::optional<std::string> CXml_File::Read(const std::string & position) {
	TXML_Position pos = CellSpec_To_TreePosition(position);
	return Read(pos);
}

std::optional<std::string> CXml_File::Read(const TXML_Position& position) {
	return mFile->Read(position);
}

bool CXml_File::Condition_Match(const TXML_Position& position) {

	for (auto& cond : position.conditions) {

		TXML_Position cpy = position;
		cpy.parameter = cond.parameterName;

		if (!cond.subtreeSpec.empty()) {
			std::copy(cond.subtreeSpec.begin(), cond.subtreeSpec.end(), std::back_inserter(cpy.hierarchy));
		}

		auto condVal = mFile->Read(cpy);
		if (!condVal.has_value() || condVal.value() != cond.conditionalValue) {
			return false;
		}
	}

	return true;
}

bool CXml_File::Position_Valid(const TXML_Position& position) {
	// TODO: more structured approach
	auto val = mFile->Read(position);
	return val.has_value();
}

void CXml_File::Write(TXML_Position& position, const std::string &value) {
	mFile->Write(position, value);
}

void CXml_File::Finalize() {
	//
}
