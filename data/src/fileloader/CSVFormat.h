/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
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
