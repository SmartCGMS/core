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
 *       monitoring", Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 */

#pragma once

#include <string>
#include <vector>
#include <map>

/*
 * Structure encapsulating single level of hierarchy (in tree)
 */
struct TreeLevelSpec
{
	TreeLevelSpec(std::string _tagName = "", size_t _position = 0) noexcept : tagName(_tagName), position(_position) { }

	// tag (or pathspec) name
	std::string tagName;
	// ordinal position of element within parent
	size_t position;

	// invalid position; indicates error or end of hierarchy
	static const size_t npos = (size_t)-1;
	// is tree level spec position valid?
	bool Valid() const { return position != TreeLevelSpec::npos; };
};

/*
 * Structure encapsulating position within hierarchy
 */
struct TreePosition
{
	// wrapper elements sequence
	std::vector<TreeLevelSpec> hierarchy;
	// selected parameter; empty if none
	std::string parameter;

	// is tree position valid?
	bool Valid() const
	{
		if (hierarchy.empty())
			return false;

		for (size_t i = 0; i < hierarchy.size(); i++)
		{
			if (!hierarchy[i].Valid())
				return false;
		}

		return true;
	};
	// clears position records
	void Clear() { hierarchy.clear(); parameter = ""; };

	// move to next element within last wrapper element
	void ToNext() { if (hierarchy.empty()) return; hierarchy[hierarchy.size() - 1].position++; };
};

/*
 * Wrapper structure for XML element
 */
struct XMLElement
{
	// element tag name
	std::string tagName;
	// element parameters
	std::map<std::string, std::string> parameters;
	// if this is a wrapper element, this is its children
	std::map<std::string, std::vector<XMLElement>> children;
};

/*
 * XML version 1.0 format reader + writer
 */
class CXML_Format
{
	private:
		// XML version tag; remains constant
		std::string mXmlVersionTag;
		// XML root element (every XML must have just one)
		XMLElement mRootElement;

		// original filename
		std::wstring mFileName;
		// something changed?
		bool mWriteFlag;

	protected:
		// loads XML from path
		bool Load(const wchar_t* path);

		// loads contents of XML element
		void Load_To_Element(XMLElement& target, std::ifstream& file);
		// parses tag name from string; destroys string in process
		std::string Parse_Tag_Name(std::string& contents);
		// parses tag contents and fills element parameters
		void Parse_Tags(XMLElement& target, std::string& contents);

		// finds element in tree; returns nullptr if not found
		XMLElement* Find_Element(TreePosition& pos);

		// strips input from quotes, if present
		std::string Strip_Quotes(std::string& input);
		// writes element to file
		void Write_To_File(XMLElement const& element, std::ofstream& file);
		// builds parameter string to be written to file
		std::string Build_Tag_Parameters_String(XMLElement const& element) const;

	public:
		CXML_Format(const wchar_t* path);
		virtual ~CXML_Format();

		// reads from position using specifier
		std::string Read(TreePosition& pos);
		// writes to position using specifier
		void Write(TreePosition& pos, std::string value);

		// was there any error?
		bool Is_Error() const;
};
