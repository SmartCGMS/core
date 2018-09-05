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

#include <map>
#include <fstream>
#include <list>

#include "FormatAdapter.h"

using FormatRecognizerRuleMap = std::map<std::string, std::list<std::string>>;

/*
 * Class used to recognize data format (not file format)
 */
class CFormat_Recognizer
{
	private:
		// all rules for all formats; primary key = format name, secondary key = cellspec, value = matched value
		std::map<std::string, FormatRecognizerRuleMap> mRuleSet;

	protected:
		// try to match file contents to data format
		bool Match(std::string format, CFormat_Adapter& file) const;

	public:
		CFormat_Recognizer();

		// adds new format recognition pattern
		void Add_Pattern(const char* formatName, const char* cellLocation, const char* content);

		// recognize data format from supplied path
		std::string Recognize_And_Open(std::wstring path, CFormat_Adapter& target) const;
		// recognize data format from supplied originalPath (to recognize file format as well) and path
		std::string Recognize_And_Open(std::wstring originalPath, std::wstring path, CFormat_Adapter& target) const;
};
