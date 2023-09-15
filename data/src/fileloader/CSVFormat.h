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

#include <vector>
#include <fstream>
#include <optional>

#include "Misc.h"
#include <scgms/rtl/FilesystemLib.h>

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
		size_t mRowsLoaded = 0;
		// error indicator
		bool mError = false;
		// original filename
		filesystem::path mFileName;
		// were contents changed? (do we need to write file?)
		bool mWriteFlag;
		// did we recently accessed cell, which is out of range?
		bool mUnkCell = false;
		// maximum column number
		size_t mMaxColumn = 0;
		// column separator
		char mSeparator = ';';
		// cache mode
		NCache_Mode mCache_Mode = NCache_Mode::Cached;

	protected:
		// lazyloading method; loads to supplied row number, if needed
		bool Load_To_Row(size_t row);

	public:
		CCSV_Format(const filesystem::path path, char separator = 0);
		virtual ~CCSV_Format();

		void Set_Cache_Mode(NCache_Mode mode);

		// reads contents of specified cell; always string since we don't convert inputs at load time
		std::optional<std::string> Read(int row, int column);
		// write to specified cell
		void Write(int row, int column, std::string value);

		// was there any error?
		bool Is_Error() const;
		// was last access out of range?
		bool Is_UnkCell_Flag() const;
};
