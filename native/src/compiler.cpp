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

#include "compiler.h"
#include "native_segment.h"
#include "descriptor.h"
#include "process.h"
#include "auto_env.h"

#include "../../../common/utils/string_utils.h"
#include "../../../common/rtl/Dynamic_Library.h"

#include <array>
#include <regex>
#include <fstream>

#if defined(__AVX512BW__) || defined(__AVX512CD__) || defined(__AVX512DQ__) || defined(__AVX512F__) || defined(__AVX512VL__) || defined(__AVX512ER__) || defined(__AVX512PF__)
#define AVX512
#ifndef __AVX2__
#define __AVX2__
#endif
#endif


struct TCompiler_Invokation {
	const wchar_t* file_name_prefix;
	const char* options;
};

const char* out_file_var = "$(output)";
const char* source_files_var = "$(source)";
const char* def_file_var = "$(export)";
const char* sdk_include_var = "$(include)";
const char* intermediate_var = "$(intermediate)";

const filesystem::path out_dir{ "out" };	//subdirectory where to place all the intermediate, object files

const std::vector<TCompiler_Invokation> compilers = {
	{L"g++",  "-O2 -fanalyzer -march=native -Weverything, -Wl,-rpath,. -fPIC -shared -save-temps=$(intermediate) -o $(output) -DSCGMS_SCRIPT -std=c++17 -lstdc++fs -lm -I $(include)  $(source)"},
	{L"clang++",  "-O2 -march=native -Wall -Wextra -Wl,-rpath,. -fPIC -shared -save-temps=$(intermediate) -o $(output) -DSCGMS_SCRIPT -std=c++17 -lstdc++fs -lm -I $(include)  $(source)"},
#ifdef AVX512
	{L"cl",  "/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /arch:AVX512 /EHsc /D \"UNICODE\" /D \"SCGMS_SCRIPT\" /I $(include) /LD /Fe: $(output) /MD $(source) /Fo:$(intermediate) /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL"}
#elif __AVX2__
	{L"cl",  "/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /arch:AVX2 /EHsc /D \"UNICODE\" /D \"SCGMS_SCRIPT\" /I $(include) /LD /Fe: $(output) /MD $(source) /Fo:$(intermediate) /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL"}
#elif __AVX__
	{L"cl",  "/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /arch:AVX /EHsc /D \"UNICODE\" /D \"SCGMS_SCRIPT\" /I $(include) /LD /Fe: $(output) /MD $(source) /Fo:$(intermediate) /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL"}
#else
	{L"cl",  "/std:c++17 /analyze /sdl /GS /guard:cf /Ox /GL /Gv /EHsc /D \"UNICODE\" /D \"SCGMS_SCRIPT\" /I $(include) /LD /Fe: $(output) /MD $(source) /Fo:$(intermediate) /link /MACHINE:X64 /DEF:$(export) /DEBUG:FULL"}
#endif
};

void replace_in_place(std::string& str, const std::string& search, const std::string& replace) {
	size_t pos = 0;

	while ((pos = str.find(search, pos)) != std::string::npos) {
		str.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

bool Compile(const filesystem::path& compiler, const filesystem::path& env_init,
	const filesystem::path& source, const filesystem::path& target_dll,
	const filesystem::path& configured_sdk_include, const std::wstring& custom_options) {

	//determine default compiler if none is provided
	filesystem::path effective_compiler = compiler;
	if (compiler.empty()) {
#if defined(_WIN32)
		effective_compiler = "cl";
#elif defined(__APPLE__)
		effective_compiler = "clang++";
#else
		effective_compiler = "g++";
#endif
	}

	std::string effective_compiler_options;
	size_t effective_compiler_index = std::numeric_limits<size_t>::max();

	//1. extract compiler's filename
	if (custom_options.empty()) {
		const std::wstring compiler_file_name = effective_compiler.filename().wstring();

		for (size_t i = 0; i < compilers.size(); i++) {
			if (compiler_file_name.rfind(compilers[i].file_name_prefix, 0) != std::wstring::npos) {
				effective_compiler_options = compilers[i].options;
				effective_compiler_index = i;
				break;
			}
		}
	}
	else
		effective_compiler_options = Narrow_WString(custom_options);

	//at this moment, custom options may be empty - but it may a that user supplied e.g.; own custom build batch file

	//2. insert the correct filenames
	const filesystem::path dst_path = target_dll.parent_path();
	const std::wstring name_prefix = source.stem().wstring();

	const filesystem::path intermediate_path = dst_path / out_dir;// / "";
	std::error_code ec;
	filesystem::create_directories(intermediate_path, ec);
	if (ec)
		return false;

	//filesystem::path effective_intermediate_dll_path{ intermediate_dll_path };
		//effective_intermediate_dll_path.replace_extension(CDynamic_Library::Default_Extension());

	const filesystem::path intermediate_dll_path = filesystem::path{ intermediate_path / target_dll.stem() }.replace_extension(CDynamic_Library::Default_Extension());

	const filesystem::path def_path = intermediate_path / (name_prefix + L"_export.def");
	const filesystem::path build_log_path = intermediate_path / (name_prefix + L"_build.log");
	

	//let's try to locate the SmartCGMS include dir
	filesystem::path sdk_include = configured_sdk_include;
	if (sdk_include.empty()) {
		//let's try to locate it
		sdk_include = Get_Dll_Dir().parent_path().parent_path().parent_path() / "common";

		std::error_code ec;
		if (!Is_Regular_File_Or_Symlink(sdk_include / "iface" / "DeviceIface.cpp")) {
			sdk_include.clear();	//no luck, let's keep it clear as originally desired	
		}
	}

	const filesystem::path device_iface_cpp = sdk_include / "iface" / "DeviceIface.cpp";
	//do not forget quotes as the path may contain a space
	const std::string complete_sources = quote(source.string()) + " " + quote(device_iface_cpp.string());

	std::string intermediate_patched_path{ intermediate_path.string() };
	//there's MSVC bug, which requires that this particular dir needs at least one forward slash
	if (effective_compiler == "cl") {
		for (auto& ch : intermediate_patched_path)
			if (ch == '\\') ch = '/';
		intermediate_patched_path += '/';	//must end as a directory

		replace_in_place(effective_compiler_options, out_file_var, quote(intermediate_patched_path));
	} else
			replace_in_place(effective_compiler_options, out_file_var, quote(intermediate_dll_path.string()));
	
	replace_in_place(effective_compiler_options, def_file_var, quote(def_path.string()));
	replace_in_place(effective_compiler_options, source_files_var, complete_sources);
	replace_in_place(effective_compiler_options, sdk_include_var, quote(sdk_include.string()));
	replace_in_place(effective_compiler_options, intermediate_var, quote(intermediate_patched_path));

	//3. delete the generated files first
	{
		std::error_code ec;
		filesystem::remove(target_dll, ec);		//returns true if deleted, false not exist => need to check ec
		if (!ec) filesystem::remove(def_path, ec);
		if (!ec) filesystem::remove(build_log_path, ec);
		if (!ec) filesystem::remove(intermediate_dll_path, ec);
		if (ec)
			return false;
	}

	//4. create the native.cpp, native.h a source.def files
	{
		{
			std::ofstream def_file{ def_path };
			def_file << "LIBRARY " << target_dll.filename().string() << std::endl << std::endl << //.string to remove quotes
				"EXPORTS" << std::endl << "\t" << native::rsScript_Entry_Symbol << std::endl
				<< "\t" << native::rsCustom_Data_Size << std::endl;
		}
	}

	//5. eventually, compile the script to dll

	std::vector<std::string> commands;
	if (!env_init.empty()) {
		commands.push_back(env_init.string());
	}
	else {
		if (effective_compiler_index != std::numeric_limits<size_t>::max()) {
			const auto default_env_init = Get_Env_Init(compilers[effective_compiler_index].file_name_prefix);
			if (!default_env_init.empty())
				commands.push_back(default_env_init);
		}
	}
	commands.push_back(effective_compiler.string() + " " + effective_compiler_options);

	std::vector<char> error_output;

#ifdef _WIN32
	const wchar_t* rsShell = L"cmd.exe";
#else
	const wchar_t* rsShell = L"/bin/sh";
#endif
	//bool result = Execute_Commands(rsShell, target_dll.parent_path().wstring(), commands, error_output);
	bool result = Execute_Commands(rsShell, intermediate_path.wstring(), commands, error_output);
	//write the log
	{
		std::ofstream error_log{ build_log_path, std::ios::binary };

		error_log << "Shell: " << Narrow_WChar(rsShell)  << std::endl;
		error_log << "Working dir: " << intermediate_path << std::endl;
		error_log << "Commands:" << std::endl;
		for (const auto &cmd : commands)
			error_log << '\t' << cmd << std::endl;
		error_log << std::endl << "Captured output:" << std::endl << std::endl;

		error_log.write(error_output.data(), error_output.size());
	}

	//do not forget to move the dll file!
	if (result) {
		//filesystem::path effective_intermediate_dll_path{ intermediate_dll_path };
		//effective_intermediate_dll_path.replace_extension(CDynamic_Library::Default_Extension());
		filesystem::rename(intermediate_dll_path, target_dll, ec);
		result = !ec.operator bool();
	}

	return result;
}