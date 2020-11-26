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

#include "native_segment.h"

#include <array>
#include <regex>

#if defined(__AVX512BW__) || defined(__AVX512CD__) || defined(__AVX512DQ__) || defined(__AVX512F__) || defined(__AVX512VL__) || defined(__AVX512ER__) || defined(__AVX512PF__)
	#define AVX512
	#ifndef __AVX2__
		#define __AVX2__
	#endif
#endif

struct TCompiler_Invokation {
	const wchar_t* file_name_prefix;
	const wchar_t* options;
};

const std::wregex out_file_var{ L"$(output)" } ;
const std::wregex source_files_var{ L"$(source)" };
const std::wregex def_file_var { L"$(export)" };

const std::array<TCompiler_Invokation, 1> compilers = {
#ifdef AVX512
	{L"cl",  L"/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /arch:AVX512 /EHsc /D \"UNICODE\" /LD /Fe: $(output) /MD $(source)  /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL}"}
#elif __AVX2__
	{L"cl",  L"/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /arch:AVX2 /EHsc /D \"UNICODE\" /LD /Fe: $(output) /MD $(source)  /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL}"}
#elif __AVX__
	{L"cl",  L"/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /arch:AVX /EHsc /D \"UNICODE\" /LD /Fe: $(output) /MD $(source)  /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL}"}
#else
	{L"cl",  L"/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /EHsc /D \"UNICODE\" /LD /Fe: $(output) /MD $(source)  /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL}"}
#endif
};

bool Compile(const filesystem::path& compiler, const filesystem::path& env_init,
			 const filesystem::path& source, const filesystem::path& dll,
			 const std::wstring& custom_options) {

	std::wstring effective_compiler_options;

	//1. extract compiler's filename
	if (custom_options.empty()) {
		const std::wstring compiler_file_name = compiler.filename();

		for (size_t i = 0; i < compilers.size(); i++) {
			if (compiler_file_name.rfind(compilers[i].file_name_prefix, 0) != std::wstring::npos) {
				effective_compiler_options = compilers[i].options;
				break;
			}
		}
	}
	else
		effective_compiler_options = custom_options;

	//at this moment, custom options may be empty - but it may a that user supplied e.g.; own custom build batch file
	
	//2. insert the correct filenames
	const filesystem::path dst_path = dll.parent_path();
	const std::wstring name_prefix = source.filename().wstring();
	
	const filesystem::path def_path = dst_path / (name_prefix + L".def");
	const filesystem::path dllmain_path = dst_path / (name_prefix + L"_native.cpp");
	const filesystem::path header_path = dst_path / (name_prefix + L"_native.h");

	effective_compiler_options = std::regex_replace(effective_compiler_options, out_file_var, dll.wstring());
	effective_compiler_options = std::regex_replace(effective_compiler_options, def_file_var, def_path.wstring());
	effective_compiler_options = std::regex_replace(effective_compiler_options, source_files_var, dllmain_path.wstring() + L" " + source.wstring());

	//3. create the native.cpp, native.h a source.def files


	//4. eventually, compile the script to dll

}