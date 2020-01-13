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

#include "../../../../common/utils/string_utils.h"

#include "XMLFormat.h"
#include "Misc.h"

#include <fstream>
#include <string>

CXML_Format::CXML_Format(const wchar_t* path) : mFileName(path), mWriteFlag(false)
{
	Load(path);
}

CXML_Format::~CXML_Format()
{
}

void CXML_Format::Write_To_File(XMLElement const& element, std::ofstream& file)
{
	// common base
	file << "<" << element.tagName << Build_Tag_Parameters_String(element);

	// no children = end tag and return
	if (element.children.empty())
	{
		file << "/>";
		return;
	}

	file << ">";

	// dump children inside
	for (auto& child : element.children)
	{
		for (size_t i = 0; i < child.second.size(); i++)
			Write_To_File(child.second[i], file);
	}

	file << "</" << element.tagName << ">";
}

std::string CXML_Format::Build_Tag_Parameters_String(XMLElement const& element) const
{
	if (element.parameters.empty())
		return "";

	std::string out = "";

	for (auto& par : element.parameters)
		out += " " + par.first + "=\"" + par.second + "\"";

	return out;
}

XMLElement* CXML_Format::Find_Element(TreePosition& pos)
{
	if (!pos.Valid())
		return nullptr;

	XMLElement* el = &mRootElement;

	// verify root element name
	if (pos.hierarchy[0].tagName != el->tagName)
		return nullptr;

	size_t i = 1;

	// descend in hierarchy
	for (; i < pos.hierarchy.size(); i++)
	{
		if (el->children[pos.hierarchy[i].tagName].size() <= pos.hierarchy[i].position)
		{
			pos.hierarchy[i].position = TreeLevelSpec::npos;
			return nullptr;
		}

		el = &el->children[pos.hierarchy[i].tagName][pos.hierarchy[i].position];
	}

	return el;
}

std::string CXML_Format::Read(TreePosition& pos)
{
	if (pos.hierarchy.size() == 0)
		return "";

	XMLElement* el = Find_Element(pos);

	if (!el)
	{
		pos.hierarchy[pos.hierarchy.size() - 1].position = TreeLevelSpec::npos;
		return "";
	}

	// probe reading; no value should be returned, but still valid attempt
	if (pos.parameter == "")
		return "";

	if (el->parameters.find(pos.parameter) == el->parameters.end())
	{
		pos.hierarchy[pos.hierarchy.size() - 1].position = TreeLevelSpec::npos;
		return "";
	}

	return el->parameters[pos.parameter];
}

void CXML_Format::Write(TreePosition& pos, std::string value)
{
	if (pos.hierarchy.size() == 0)
		return;

	XMLElement* el = Find_Element(pos);

	if (!el)
	{
		pos.hierarchy[pos.hierarchy.size() - 1].position = TreeLevelSpec::npos;
		return;
	}

	if (pos.parameter == "")
		return;

	if (el->parameters.find(pos.parameter) == el->parameters.end())
	{
		pos.hierarchy[pos.hierarchy.size() - 1].position = TreeLevelSpec::npos;
		return;
	}

	mWriteFlag = true;

	el->parameters[pos.parameter] = value;
}

bool CXML_Format::Is_Error() const
{
	return false;
}

bool CXML_Format::Load(const wchar_t* path)
{	
	std::ifstream f(Narrow_WChar(path));

	if (!f.is_open())
		return false;

	Discard_BOM_If_Present(f);

	std::string el;

	// automatically discard trailing mess
	std::getline(f, el, '<');
	std::getline(f, el, '>');

	// this element should contain XML version tag

	if (el.substr(0, 4) == "?xml" && el.substr(el.length() - 1) == "?")
	{
		// store to allow saving with unchanged version tag
		mXmlVersionTag = "<" + el + ">";

		// prepare reading of root element
		std::getline(f, el, '<'); // discard out-of-tag mess
		std::getline(f, el, '>'); // read tag
	}
	else
		mXmlVersionTag = "<?xml version=\"1.0\"?>";

	mRootElement.tagName = Parse_Tag_Name(el);

	// parse root element contents
	Parse_Tags(mRootElement, el);

	// recursivelly parse root element contents
	Load_To_Element(mRootElement, f);

	return true;
}

void CXML_Format::Load_To_Element(XMLElement& target, std::ifstream& file)
{
	std::string el, tagName;

	while (!file.eof() && !file.bad())
	{
		std::getline(file, el, '<'); // discard out-of-tag mess
		std::getline(file, el, '>'); // read tag

		// closing tag of current "parent" - return
		if (el == "/" + target.tagName)
			break;

		tagName = Parse_Tag_Name(el);
		if (tagName.empty())
			continue;

		target.children[tagName].push_back(XMLElement());

		XMLElement& xmlel = target.children[tagName][target.children[tagName].size() - 1];
		xmlel.tagName = tagName;

		Parse_Tags(xmlel, el);

		// recursive step
		if (el.length() == 0 || el[el.length() - 1] != '/')
			Load_To_Element(xmlel, file);
	}
}

std::string CXML_Format::Parse_Tag_Name(std::string& contents)
{
	size_t pos = contents.find_first_of(" /");

	std::string tagName = "";

	if (pos == std::string::npos)
	{
		tagName = contents;
		contents.clear();
	}
	else
	{
		tagName = contents.substr(0, pos);
		contents = contents.substr(pos + 1);
	}

	return tagName;
}

void CXML_Format::Parse_Tags(XMLElement& target, std::string& contents)
{
	// empty parameter list
	if (contents.empty())
		return;

	size_t lpos, epos, qpos;

	while (true)
	{
		lpos = contents.find_first_not_of(' ');

		// also empty
		if (lpos == '/' || lpos == std::string::npos)
			return;

		epos = contents.find('=', lpos);

		if (epos == std::string::npos)
			return;

		qpos = contents.find('\"', epos + 2);

		if (qpos == std::string::npos)
			return;

		target.parameters[contents.substr(lpos, epos - lpos)] = contents.substr(epos + 2, qpos - epos - 2);

		if (qpos + 1 == contents.length())
			return;

		contents = contents.substr(qpos + 1);
	}
}

std::string CXML_Format::Strip_Quotes(std::string& input)
{
	if (input.length() < 2)
		return input;

	if (input[0] == '\"' && input[input.length() - 1] == '\"')
		return input.substr(1, input.length() - 2);

	return input;
}
