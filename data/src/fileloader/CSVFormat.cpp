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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "CSVFormat.h"
#include "../../../../common/utils/winapi_mapping.h"

#include <map>
#include <sstream>
#include <cstdio>
#include <errno.h>

enum class CSVState
{
	UnquotedField,
	QuotedField,
	QuotedQuote
};

CCSV_Format::CCSV_Format(const filesystem::path path, char separator) : mRowsLoaded(0), mError(0), mFileName(path), mWriteFlag(false), mMaxColumn(0)
{
	mFile.open(path);
	if (mFile.bad())
	{
		mError = 1;
		return;
	}

	Discard_BOM_If_Present(mFile);

	// save initial position in case BOM was discarded
	std::streampos initPos = mFile.tellg();

	// if the separator wasn't specified, try to recognize it using frequency map
	if (separator == 0)
	{
		std::map<char, size_t> freqMap = { { ';', 0 },{ '\t', 0 },{ ',', 0 } };

		std::string tmp;
		while (std::getline(mFile, tmp) && tmp.length() < 3)
			;

		if (!tmp.empty())
		{
			for (size_t i = 0; i < tmp.length(); i++)
			{
				if (freqMap.find(tmp[i]) != freqMap.end())
					freqMap[tmp[i]]++;
			}

			auto itr = freqMap.begin();
			auto maxItr = itr;
			for (; itr != freqMap.end(); ++itr)
			{
				if (itr->second > maxItr->second)
					maxItr = itr;
			}

			separator = maxItr->first;
		}
		else
			separator = ';';

		// reset stream to its original position
		mFile.clear();
		mFile.seekg(initPos);
	}

	mSeparator = separator;
}

CCSV_Format::~CCSV_Format()
{
}

void CCSV_Format::Set_Cache_Mode(NCache_Mode mode) {
	mCache_Mode = mode;

	// either way, clear the cache
	mContents.clear();
}

std::optional<std::string> CCSV_Format::Read(int row, int column)
{
	if (mError)
		return std::nullopt;

	// lazyload if needed
	bool read = Load_To_Row(static_cast<size_t>(row));

	const int realRowIdx = (mCache_Mode == NCache_Mode::Cached) ? row : 0;

	// if there are not enough rows / columns after lazyload, it means the cell does not exist
	// note that we always consider maximum number of columns ever read; this is because of segmented CSVs, etc.
	if (!read || static_cast<int>(mContents.size()) <= realRowIdx || static_cast<int>(mMaxColumn) <= column)
	{
		mUnkCell = true;
		return std::nullopt;
	}

	if (static_cast<int>(mContents[realRowIdx].size()) <= column)
		return std::nullopt;

	mUnkCell = false;
	return mContents[realRowIdx][column];
}

void CCSV_Format::Write(int row, int column, std::string value)
{
	if (mError || row < 0)
		return;

	// we do not support writing in single-record cache
	if (mCache_Mode == NCache_Mode::Single_Record_Cache) {
		mError = true;
		return;
	}

	// lazyload if needed
	/*bool read_unused = */Load_To_Row(static_cast<size_t>(row));

	const int realRowIdx = row;

	// resize if needed
	if (static_cast<int>(mContents.size()) <= realRowIdx)
		mContents.resize(static_cast<size_t>(realRowIdx) + 1);
	if (static_cast<int>(mContents[realRowIdx].size()) <= column)
		mContents[row].resize(static_cast<size_t>(column) + 1);

	// write only if needed
	if (mContents[realRowIdx][column] != value)
	{
		mContents[realRowIdx][column] = value;
		mWriteFlag = true; // set writeflag to indicate file change
	}
}

bool CCSV_Format::Load_To_Row(size_t row)
{
	// we load more rows only if needed
	if (mRowsLoaded <= row)
	{
		CSVState state = CSVState::UnquotedField;

		std::string line;
		while (mRowsLoaded <= row)
		{
			if (!std::getline(mFile, line)) {
				return false;
			}

			std::vector<std::string> tok{""};

			size_t i = 0;

			for (char c : line)
			{
				switch (state)
				{
					case CSVState::UnquotedField:
						switch (c)
						{
							case ',': // end of field
							case ';':
							case '\t':
								if (c == mSeparator)
								{
									tok.push_back("");
									i++;
								}
								else
									tok[i].push_back(c);
								break;
							case '"':
								state = CSVState::QuotedField;
								break;
							default:
								tok[i].push_back(c);
								break;
						}
						break;
					case CSVState::QuotedField:
						switch (c)
						{
							case '"':
								state = CSVState::QuotedQuote;
								break;
							default:
								tok[i].push_back(c);
								break;
						}
						break;
					case CSVState::QuotedQuote:
						switch (c)
						{
							case ',': // , after closing quote
							case ';':
							case '\t':
								if (c == mSeparator)
								{
									tok.push_back("");
									i++;
									state = CSVState::UnquotedField;
								}
								break;
							case '"': // "" -> "
								tok[i].push_back('"');
								state = CSVState::QuotedField;
								break;
							default:  // end of quote
								state = CSVState::UnquotedField;
								break;
						}
						break;
				}
			}

			size_t realRowIdx = (mCache_Mode == NCache_Mode::Cached) ? mRowsLoaded : 0;

			mContents.resize(realRowIdx + 1);
			mContents[realRowIdx].assign(tok.begin(), tok.end());
			mMaxColumn = mMaxColumn >= mContents[realRowIdx].size() ? mMaxColumn : mContents[realRowIdx].size();

			mRowsLoaded++;
		}
	}

	return true;
}

bool CCSV_Format::Is_Error() const
{
	return mError;
}

bool CCSV_Format::Is_UnkCell_Flag() const
{
	return mUnkCell;
}
