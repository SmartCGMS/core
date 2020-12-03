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

#include "process.h"

#include <rtl/FilesystemLib.h>

#include <stdio.h> 
#include <strsafe.h>
#include <string>
#include <codecvt>
#include <iostream>
#include <future>

#ifdef _WIN32
	#include <windows.h> 
	
	using TPipe_Handle = HANDLE;
#endif

struct TPipe {
	TPipe_Handle input, output;
};

struct TRedirected_IO {
	TPipe input, error;
	TPipe_Handle output;	//we'll discard the stdout because we care just for the error detection
};

class CProcess {
protected:
	std::vector<char> &mOutput;
	TRedirected_IO mIO;
#ifdef _WIN32
	PROCESS_INFORMATION mProcess;
#endif
protected:
	bool Setup_IO() {	
#ifdef _WIN32
		SECURITY_ATTRIBUTES saAttr;
		// Set the bInheritHandle flag so pipe handles are inherited. 
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		
		bool result = CreatePipe(&mIO.input.output, &mIO.input.input, &saAttr, 0) == TRUE;
		result &= CreatePipe(&mIO.error.output, &mIO.error.input, &saAttr, 0) == TRUE;

		//we are not interested in the standard output, we care only for the errors
		if (result)
			mIO.output = CreateFileW(L"nul:", GENERIC_READ | GENERIC_WRITE,
			                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
											 NULL, OPEN_EXISTING, 0, NULL);
		result &= mIO.output != INVALID_HANDLE_VALUE;
		
		if (result) {
			//And ensure that those ends of pipes, which we use to read and write in our process, are inherited
			result &= SetHandleInformation(mIO.input.input, HANDLE_FLAG_INHERIT, 0) == TRUE;
			result &= SetHandleInformation(mIO.input.output, HANDLE_FLAG_INHERIT, 0) == TRUE;
			result &= SetHandleInformation(mIO.error.input, HANDLE_FLAG_INHERIT, 0) == TRUE;
			result &= SetHandleInformation(mIO.error.output, HANDLE_FLAG_INHERIT, 0) == TRUE;
			result &= SetHandleInformation(mIO.output, HANDLE_FLAG_INHERIT, 0) == TRUE;
		}
#else
		result = false;
#endif

		return result;
	}

	bool Create_Shell(const std::wstring &shell, const std::wstring &working_dir) { // Create a child process that uses the previously created pipes for STDIN and STDOUT.	
																										   //command must be string to keep it allocated
#ifdef _WIN32		
		STARTUPINFO siStartInfo;		

		// Set up members of the PROCESS_INFORMATION structure. 

		memset(&mProcess, 0, sizeof(PROCESS_INFORMATION));

		// Set up members of the STARTUPINFO structure. 
		// This structure specifies the STDIN and STDOUT handles for redirection.

		memset(&siStartInfo, 0, sizeof(STARTUPINFO));
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = mIO.error.output;
		siStartInfo.hStdOutput = mIO.output;
		siStartInfo.hStdInput = mIO.input.input;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
				
		const LPWSTR working_dir_raw = working_dir.empty() ? NULL : (LPWSTR)working_dir.c_str();

		// Create the child process. 	
		const bool result = CreateProcessW(NULL,	
			(LPWSTR)shell.c_str(),     // command line 
			NULL,
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			0,             // creation flags 
			NULL,          // use parent's environment 
			working_dir_raw,
			&siStartInfo,  // STARTUPINFO pointer 
			&mProcess) == TRUE;  // receives PROCESS_INFORMATION 

		if (!result) mProcess.hThread = mProcess.hProcess = (HANDLE)INVALID_HANDLE_VALUE;
		//assign to both not to potenatially leak important information
#else
		result = false;
#endif
		return result;
	}

		
	bool Write_Input(const std::vector<std::string> &input) {
#ifdef _WIN32
		DWORD dwWritten;
		for (size_t i = 0; i < input.size(); i++) {
			// Stop if there are no more data. 

			if (!WriteFile(mIO.input.input, input[i].data(), static_cast<DWORD>(input[i].size()), &dwWritten, NULL)) return false;

			const char endl[2] = { 0xd, 0xa };
			WriteFile(mIO.input.input, endl, sizeof(endl), &dwWritten, NULL);

		}

		// Close the pipe handle so the child process stops reading. 
		return CloseHandle(mIO.input.input);
#else
		return false;
#endif
	}


	bool Read_Ouput() {
#ifdef _WIN32
		const HANDLE &read = mIO.error.output;
		HANDLE objects[2] = { mProcess.hProcess, read };

		const size_t buffer_size = 4096;

		DWORD dwRead;
		char chBuf[buffer_size];

		for (;;) {

			//Check if we can read data and if we can, let's read them
			if (PeekNamedPipe(read, NULL, 0, NULL, &dwRead, NULL)) {
				if (dwRead>0)
					if (ReadFile(read, chBuf, buffer_size, &dwRead, NULL))
						mOutput.insert(mOutput.end(), chBuf, chBuf + dwRead);
			}

			//wait until the process has ended or new data are available
			if (WaitForMultipleObjects(2, objects, false, INFINITE) == WAIT_OBJECT_0)
				break;

		}

		return true;
#else
		return false;
#endif
	}

	void Clean_Up() {		
#ifdef _WIN32
		CloseHandle(mProcess.hThread);
		CloseHandle(mProcess.hProcess);		
#endif
	}


	bool Succeeded_by_Return_Code() {
#ifdef _WIN32
		DWORD ec = DWORD(-1);
		if (!GetExitCodeProcess(mProcess.hProcess, &ec)) ec = DWORD(-1);
		return ec == 0;		
#else
		return false;
#endif
	}


public:
	CProcess(std::vector<char>& output) : mOutput(output) {}

	bool run(const std::wstring& shell, const std::wstring& working_dir, std::vector<std::string> input) {
		bool result = false;	//assume false
		

		try {
			if (Setup_IO()) {
				if (Create_Shell(shell, working_dir)) {
					auto read_result = std::async(&CProcess::Read_Ouput, this);
					
					if (Write_Input(input) && read_result.get())
						result = Succeeded_by_Return_Code();					
				}
			}
		}
		catch (...) {
			result = false;
		}		

		//and clean-up
		Clean_Up();

		return result;
	}

};


bool Execute_Commands(const std::wstring& shell, const std::wstring &working_dir, const std::vector<std::string>& commands, std::vector<char>& output) {
	CProcess proceses{ output };
	return proceses.run(shell, working_dir, commands);
}