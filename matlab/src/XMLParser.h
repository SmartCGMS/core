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

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <codecvt>
#include <locale>

// TODO: define templates (maybe variadic) for char16_t, char32_t, ..

template<typename C> constexpr const C* ChooseStrCW(const char* c, const wchar_t* w);
template<> constexpr const char* ChooseStrCW<char>(const char* c, const wchar_t* w) { return c; }
template<> constexpr const wchar_t* ChooseStrCW<wchar_t>(const char* c, const wchar_t* w) { return w; }

#define StrCW(C, STR) ChooseStrCW<C>(STR, L##STR)

template<typename C> constexpr const C ChooseCW(const char c, const wchar_t w);
template<> constexpr const char ChooseCW<char>(const char c, const wchar_t w) { return c; }
template<> constexpr const wchar_t ChooseCW<wchar_t>(const char c, const wchar_t w) { return w; }

#define CW(C, STR) ChooseCW<C>(STR, L##STR)

template<typename T, typename S = std::basic_string<T, std::char_traits<T>, std::allocator<T>>>
struct CXML_Element
{
	S name;
	std::map<S, S> parameters;
	std::map<S, std::vector<CXML_Element<T>>> children;

	S const& Get_Parameter(const S& key) {
		const auto& itr = parameters.find(key);
		if (itr == parameters.end())
			throw std::runtime_error("Requested parameter not found");
		return itr->second;
	}
	S const& Get_Parameter(const S& key, const S& defaultValue) {
		try {
			return Get_Parameter(key);
		} catch (...) {
			return defaultValue;
		}
	}
};

template<typename T, typename S = std::basic_string<T, std::char_traits<T>, std::allocator<T>>,
	typename IFS = std::basic_ifstream<T, std::char_traits<T>>, typename ISS = std::basic_istringstream<T, std::char_traits<T>, std::allocator<T>>>
class CXML_Parser
{
	protected:
		static constexpr const T rsQuote = CW(T, '"');
		static constexpr const T rsLAngle = CW(T, '<');
		static constexpr const T rsRAngle = CW(T, '>');
		static constexpr const T rsFWSlash = CW(T, '/');
		static constexpr const T rsEquals = CW(T, '=');
		static constexpr const T rsDot = CW(T, '.');
		static constexpr const T rsColon = CW(T, ':');
		static constexpr const T* rsWhitespaces = StrCW(T, " \t\r\n");
		static constexpr const T* rsQuestionSubstr = StrCW(T, "?");
		static constexpr const T* rsXMLOpeningSubstr = StrCW(T, "?xml");
		static constexpr const T* rsFWSlashSpaceSubstr = StrCW(T, " /");
		static constexpr const T* rsEmpty = StrCW(T, "");
		static constexpr const T* rsCommentStartSubstr = StrCW(T, "!--");

	protected:
		CXML_Element<T> mRootElement;

		IFS mFile;
		bool mValid;

		void Parse_Root_Element() {
			S el;
			std::getline(mFile, el, rsLAngle); std::getline(mFile, el, rsRAngle);
			if (el.substr(0, 4) == rsXMLOpeningSubstr && el.substr(el.length() - 1) == rsQuestionSubstr) {
				std::getline(mFile, el, rsLAngle); std::getline(mFile, el, rsRAngle);
			}
			S rname = Parse_Tag_Name(el);
			if (rname.empty())
				return;
			mValid = true;
			mRootElement.name = rsEmpty;
			mRootElement.children[rname].push_back({});
			CXML_Element<T>& element = *mRootElement.children[rname].rbegin();
			element.name = rname;
			Parse_Tags(element, el);
			Load_To_Element(element);
		}

		S Strip_Quotes(const S& input) const {
			if (input.length() < 2)
				return input;
			if (input[0] == rsQuote && input[input.length() - 1] == rsQuote)
				return input.substr(1, input.length() - 2);
			return input;
		}

		S Parse_Tag_Name(S& contents) const {
			size_t pos = contents.find_first_of(rsFWSlashSpaceSubstr);
			S tagName = rsEmpty;
			if (pos == S::npos) {
				tagName = contents;
				contents.clear();
			} else {
				tagName = contents.substr(0, pos);
				contents = contents.substr(pos + 1);
			}
			return tagName;
		}

		void Load_To_Element(CXML_Element<T>& target) {
			S el, tagName;
			while (!mFile.eof() && !mFile.bad()) {
				std::getline(mFile, el, rsLAngle); std::getline(mFile, el, rsRAngle);
				if (el == rsFWSlash + target.name)
					break;
				tagName = Parse_Tag_Name(el);
				if (tagName.empty() || tagName.substr(0, 3) == rsCommentStartSubstr)
					continue;
				target.children[tagName].push_back({});
				CXML_Element<T>& xmlel = *target.children[tagName].rbegin();
				xmlel.name = tagName;
				Parse_Tags(xmlel, el);
				if (el.length() == 0 || el[el.length() - 1] != rsFWSlash)
					Load_To_Element(xmlel);
			}
		}

		void Parse_Tags(CXML_Element<T>& target, S& contents) const {
			if (contents.empty())
				return;
			size_t lpos, epos, qpos;
			while (true)
			{
				lpos = contents.find_first_not_of(rsWhitespaces);
				if (contents[lpos] == rsFWSlash || lpos == S::npos)
					return;
				epos = contents.find(rsEquals, lpos);
				if (epos == S::npos)
					return;
				qpos = contents.find(rsQuote, epos + 2);
				if (qpos == S::npos)
					return;
				target.parameters[contents.substr(lpos, epos - lpos)] = contents.substr(epos + 2, qpos - epos - 2);
				if (qpos + 1 == contents.length())
					return;
				contents = contents.substr(qpos + 1);
			}
		}

	public:
		CXML_Parser(const S& fileName) noexcept : mValid(false) {
			std::wstring_convert<std::codecvt_utf8<T>> myconv;
			mFile.open(myconv.to_bytes(fileName));
			if (mFile.is_open()) Parse_Root_Element();
		}

		virtual ~CXML_Parser() {
		}

		bool Is_Valid() const {
			return mValid;
		}

		CXML_Element<T> const& Get_Element(const S& path) const {
			ISS iss(path); S tag, param;

			CXML_Element<T> const* el = &mRootElement;
			while (std::getline(iss, tag, rsDot)) {
				auto scpos = tag.find_first_of(rsColon);
				if (scpos != std::wstring::npos) {
					param = tag.substr(scpos + 1);
					tag = tag.substr(0, scpos);
				}

				auto itr = el->children.find(tag);
				if (itr == el->children.end() || itr->second.empty())
					throw std::runtime_error("Requested element not found");

				el = &itr->second[0];
			}

			return *el;
		}

		S const& Get_Parameter(const S& path) const {
			ISS iss(path); S tag, param;

			CXML_Element<T> const* el = &mRootElement;
			while (std::getline(iss, tag, rsDot)) {
				auto scpos = tag.find_first_of(rsColon);
				if (scpos != std::wstring::npos) {
					param = tag.substr(scpos + 1);
					tag = tag.substr(0, scpos);
				}
				if (!tag.empty()) {
					auto itr = el->children.find(tag);
					if (itr == el->children.end() || itr->second.empty())
						throw std::runtime_error("Requested element not found");

					el = &itr->second[0];
				}
				if (!param.empty()) {
					auto pitr = el->parameters.find(param);
					if (pitr == el->parameters.end())
						throw std::runtime_error("Requested parameter not found");
					return pitr->second;
				}
			}
			throw std::runtime_error("Requested parameter not found");
		}

		S const& Get_Parameter(const S& path, const S& defaultValue) const {
			try {
				return Get_Parameter(path);
			} catch (...) {
				return defaultValue;
			}
		}
};
