#include "CSVFormat.h"
#include "Misc.h"

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

CCSV_Format::CCSV_Format(const wchar_t* path, char separator) : mFile(path), mRowsLoaded(0), mError(0), mFileName(path), mWriteFlag(false), mMaxColumn(0)
{
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

std::string CCSV_Format::Read(int row, int column)
{
	if (mError)
		return "";

	// lazyload if needed
	Load_To_Row(row);

	// if there are not enough rows / columns after lazyload, it means the cell does not exist
	// note that we always consider maximum number of columns ever read; this is because of segmented CSVs, etc.
	if (mContents.size() <= row || mMaxColumn <= column)
	{
		mUnkCell = true;
		return "";
	}

	if (mContents[row].size() <= column)
		return "";

	mUnkCell = false;
	return mContents[row][column];
}

void CCSV_Format::Write(int row, int column, std::string value)
{
	if (mError)
		return;

	// lazyload if needed
	Load_To_Row(row);

	// resize if needed
	if (mContents.size() <= row)
		mContents.resize(row + 1);
	if (mContents[row].size() <= column)
		mContents[row].resize(column + 1);

	// write only if needed
	if (mContents[row][column] != value)
	{
		mContents[row][column] = value;
		mWriteFlag = true; // set writeflag to indicate file change
	}
}

void CCSV_Format::Load_To_Row(int row)
{
	// we load more rows only if needed
	if (mRowsLoaded <= row)
	{
		CSVState state = CSVState::UnquotedField;

		std::string line;
		while (mRowsLoaded <= row && std::getline(mFile, line))
		{
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

			mContents.resize(mRowsLoaded + 1);
			mContents[mRowsLoaded].assign(tok.begin(), tok.end());
			mMaxColumn = mMaxColumn >= mContents[mRowsLoaded].size() ? mMaxColumn : mContents[mRowsLoaded].size();

			mRowsLoaded++;
		}
	}
}

bool CCSV_Format::Is_Error() const
{
	return mError;
}

bool CCSV_Format::Is_UnkCell_Flag() const
{
	return mUnkCell;
}
