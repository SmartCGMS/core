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

#pragma once

#include <vector>
#include <fstream>

/**
 * CSV file reader + writer, implements lazyloading in both directions
 */
class CCSV_Format
{
	private:
		// CSV contents; primary = row, secondary = column
		std::vector<std::vector<std::string>> mContents;
		// opened file stream
		std::ifstream mFile;
		// number of loaded rows
		size_t mRowsLoaded;
		// error indicator
		bool mError;
		// original filename
		std::wstring mFileName;
		// were contents changed? (do we need to write file?)
		bool mWriteFlag;
		// did we recently accessed cell, which is out of range?
		bool mUnkCell;
		// maximum column number
		size_t mMaxColumn;
		// column separator
		char mSeparator;

	protected:
		// lazyloading method; loads to supplied row number, if needed
		void Load_To_Row(size_t row);

	public:
		CCSV_Format(const wchar_t* path, char separator = 0);
		virtual ~CCSV_Format();

		// reads contents of specified cell; always string since we don't convert inputs at load time
		std::string Read(int row, int column);
		// write to specified cell
		void Write(int row, int column, std::string value);

		// was there any error?
		bool Is_Error() const;
		// was last access out of range?
		bool Is_UnkCell_Flag() const;
};
