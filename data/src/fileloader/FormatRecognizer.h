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

#include <map>
#include <fstream>
#include <list>

#include "FormatAdapter.h"
#include "../../../../common/rtl/FilesystemLib.h"

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
		std::string Recognize_And_Open(filesystem::path path, CFormat_Adapter& target) const;
		// recognize data format from supplied originalPath (to recognize file format as well) and path
		std::string Recognize_And_Open(filesystem::path originalPath, filesystem::path path, CFormat_Adapter& target) const;
};
