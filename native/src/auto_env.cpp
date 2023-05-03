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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "auto_env.h"

#include "process.h"
#include "../../../common/utils/string_utils.h"

#include <array>

#include <stdlib.h>

#ifdef _WIN32
std::string Get_VS_Installation_Path(std::string vswhere, const char* tool, const char* tool_switch, const std::string &install_dir_id) {
	std::string result;
	std::vector<char> output;	
	vswhere.append(tool);
	vswhere = quote(vswhere);
	vswhere.append(" ");
	vswhere.append(tool_switch);
			

	const std::vector<std::string> commands{ vswhere};
	const wchar_t* rsShell = L"cmd.exe";
	if (Execute_Commands(rsShell, L".", commands, output)) {
		output.push_back(0);	//ensure ASCIIZ
 			
		char* install_dir_val = strstr(output.data(), install_dir_id.c_str());
		if (install_dir_val) {
			install_dir_val += install_dir_id.size();
			char* install_dir_val_end = install_dir_val;

			//until EOL (on any OS, or null-terminating char)
			while ((*install_dir_val_end != 10) && (*install_dir_val_end != 13) && (*install_dir_val_end != 0))
				install_dir_val_end++;

			result.assign(install_dir_val, install_dir_val_end);
		}
	}


	return result;
}

std::string Visual_Studio() {
	std::string result;

	std::array<char, 256> program_files_x86;
	size_t dummy_len;
	if (getenv_s(&dummy_len, program_files_x86.data(), program_files_x86.size(), "ProgramFiles(x86)") == 0) {		
		std::string vswhere_root{ program_files_x86.data() };		
		if (!vswhere_root.empty())
			vswhere_root.append("\\Microsoft Visual Studio\\Installer\\");

		result = Get_VS_Installation_Path(vswhere_root, "vswhere.exe",  "-latest", "installationPath: ");
		if (result.empty())
			result = Get_VS_Installation_Path(vswhere_root, "vs_layout.exe", "instance", "InstallationPath = ");


		if (!result.empty()) {
			result.append("\\VC\\Auxiliary\\Build\\vcvarsall.bat");
			result = quote(result) + " ";
		} else {
			result = "vcvarsall.bat ";	//the best we can do, in such a case, is to hope for correctly set %PATH%
										//trailing white space is OK
		}

#if defined(_M_AMD64) || defined(_M_X64)
			result += "x64";
#elif defined(_M_IX86)
			result += "x86";
#else
			result += "arm64";
#endif
		}

	return result;
}
#endif // _WIN32

std::string Get_Env_Init(const std::wstring& compiler_prefix) {
	std::string result;

#ifdef _WIN32
	if ((compiler_prefix == L"cl") || (compiler_prefix == L"clang++")) {
		//Let's attempt to determine Visual Studio installation
		result = Visual_Studio();
	}
#endif

	return result;
}
