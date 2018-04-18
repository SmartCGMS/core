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
		void Load_To_Row(int row);

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
