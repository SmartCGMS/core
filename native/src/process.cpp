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

#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/utils/string_utils.h"

#include <cstdio> 
#include <string>
#include <iostream>
#include <thread>

#ifdef _WIN32
	#include <strsafe.h>
	#include <windows.h>
	
	using TPipe_Handle = HANDLE;
#else
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <unistd.h>

	using TPipe_Handle = int;
#endif


struct TPipe {
	union {		
		struct {
			TPipe_Handle read, write;
		};
		
		TPipe_Handle fd[2] = { 0, 0 };				
	};

	static constexpr size_t read_idx = 0;
	static constexpr size_t write_idx = 1;
};	


struct TRedirected_IO {
	TPipe input, out_err;	
};

class CProcess {
protected:
	std::vector<char> &mOutput;
	TRedirected_IO mIO;
#ifdef _WIN32
	PROCESS_INFORMATION mProcess {0};
#else
	pid_t pid;
#endif
protected:
	bool Setup_IO() {
		bool result = false;
#ifdef _WIN32
		SECURITY_ATTRIBUTES saAttr;
		// Set the bInheritHandle flag so pipe handles are inherited. 
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		
		result = CreatePipe(&mIO.input.read, &mIO.input.write, &saAttr, 0) == TRUE;
		result &= CreatePipe(&mIO.out_err.read, &mIO.out_err.write, &saAttr, 0) == TRUE;		
		
		//And ensure that those ends of pipes, which we use to read and write in our process, are not inherited
		result &= SetHandleInformation(mIO.input.write, HANDLE_FLAG_INHERIT, 0) == TRUE;
		result &= SetHandleInformation(mIO.out_err.read, HANDLE_FLAG_INHERIT, 0) == TRUE;		
			//do not make sure of the same for the other pipe ends
#else
		result = (pipe(mIO.input.fd) == 0) && (pipe(mIO.out_err.fd) == 0);
#endif

		return result;
	}

	bool Create_Shell(const std::wstring &shell, const std::wstring &working_dir) { // Create a child process that uses the previously created pipes for STDIN and STDOUT.	
																										   //command must be string to keep it allocated
#ifdef _WIN32		
		STARTUPINFOW siStartInfo;		

		// Set up members of the PROCESS_INFORMATION structure. 

		memset(&mProcess, 0, sizeof(PROCESS_INFORMATION));

		// Set up members of the STARTUPINFO structure. 
		// This structure specifies the STDIN and STDOUT handles for redirection.

		memset(&siStartInfo, 0, sizeof(STARTUPINFOW));
		siStartInfo.cb = sizeof(STARTUPINFOW);
		siStartInfo.hStdError = mIO.out_err.write;
		siStartInfo.hStdOutput = mIO.out_err.write;
		siStartInfo.hStdInput = mIO.input.read;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
				
		const LPWSTR working_dir_raw = working_dir.empty() ? NULL : (LPWSTR)working_dir.c_str();
		std::wstring shell_buf = shell;	//CreateProcessW explicitly requires lpCmdLine to be read-write

		// Create the child process. 	
		const bool result = CreateProcessW(NULL,	
			(LPWSTR)shell_buf.c_str(),     // command line 
			NULL,
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			0,             // creation flags 
			NULL,          // use parent's environment 
			working_dir_raw,
			&siStartInfo,  // STARTUPINFO pointer 
			&mProcess) == TRUE;  // receives PROCESS_INFORMATION 

		if (!result) mProcess.hThread = mProcess.hProcess = (HANDLE)INVALID_HANDLE_VALUE;
		//assign to both not to potentially leak important information

		return result;
#else

		pid = fork();
		if (pid > 0) {
			//parent branch
			//parent reads no input nor writes any output
			close(mIO.input.fd[TPipe::read_idx]);
			close(mIO.out_err.fd[TPipe::write_idx]);
			return true;
		}
		else if (pid == 0) {
			dup2(mIO.input.fd[TPipe::read_idx], STDIN_FILENO);
			dup2(mIO.out_err.fd[TPipe::write_idx], STDOUT_FILENO);
			dup2(mIO.out_err.fd[TPipe::write_idx], STDERR_FILENO);

			close(mIO.input.fd[TPipe::write_idx]);   // Child does not write to stdin
			close(mIO.out_err.fd[TPipe::read_idx]);   // Child does not read from stdout not stderr

			//change the working dir
			if (!working_dir.empty()) {
				const std::string narrow_dir = Narrow_WString(working_dir);
				const auto rc = chdir(narrow_dir.c_str());
				if (rc != 0)
					exit(rc);
			}


			const filesystem::path shell_path{ shell };
			const std::string narrow_shell_path = Narrow_WString(shell);
			const std::string narrow_shell_cmd = shell_path.filename().string();

			execl(narrow_shell_path.c_str(), narrow_shell_cmd.c_str(), nullptr);
			exit(EXIT_FAILURE);	//execl returns only if it failed
		} else {
			//failed to fork a new process
			return false;
		}
#endif
	}


	bool Write_Input(const std::vector<std::string> &input) {
#ifdef _WIN32
		DWORD dwWritten;
		for (size_t i = 0; i < input.size(); i++) {
			// Stop if there are no more data. 

			if (!WriteFile(mIO.input.write, input[i].data(), static_cast<DWORD>(input[i].size()), &dwWritten, NULL)) 
				return false;

			const char endl[2] = { 0xd, 0xa };
			WriteFile(mIO.input.write, endl, sizeof(endl), &dwWritten, NULL);

		}

		// Close the pipe handle so the child process stops reading. 
		return CloseHandle(mIO.input.write) == TRUE;
#else

		bool succeeded = true;

		for (size_t i = 0; i < input.size(); i++) {
			// Stop if there are no more data. 

			if (write(mIO.input.fd[TPipe::write_idx], input[i].data(), input[i].size()) < 0) {
				succeeded = false;
				break;
			}

			const char endl = 0xa;	//well, it its \n even for a Mac in way we opened this file descriptor
			write(mIO.input.fd[TPipe::write_idx], &endl, 1);
		}

		return (close(mIO.input.fd[TPipe::write_idx]) == 0) && succeeded;
#endif
	}


	bool Read_Ouput() {
		const size_t buffer_size = 4096;
		char chBuf[buffer_size];


#ifdef _WIN32
		const HANDLE &read = mIO.out_err.read;
		HANDLE objects[2] = { mProcess.hProcess, read };

		DWORD dwRead;

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
		for (;;) {
			const auto bytes = read(static_cast<int>(mIO.out_err.fd[TPipe::read_idx]), chBuf, buffer_size);
			if (bytes > 0) {
				mOutput.insert(mOutput.end(), chBuf, chBuf + bytes);
			}
			else
				break;
		}		

		return true;
#endif
	}

	void Clean_Up() {		
#ifdef _WIN32
		CloseHandle(mProcess.hThread);
		CloseHandle(mProcess.hProcess);		
#else
		close(mIO.input.fd[TPipe::read_idx]);
		close(mIO.input.fd[TPipe::write_idx]);

		close(mIO.out_err.fd[TPipe::read_idx]);
		close(mIO.out_err.fd[TPipe::write_idx]);
#endif
	}


	bool Succeeded_by_Return_Code() {
#ifdef _WIN32
		DWORD ec = DWORD(-1);
		if (!GetExitCodeProcess(mProcess.hProcess, &ec)) ec = DWORD(-1);
		return ec == 0;		
#else
		int status = 0;
		waitpid(pid, &status, 0);
		
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status) == EXIT_SUCCESS;
		}
			

		return false;
#endif
	}


public:
	CProcess(std::vector<char>& output) : mOutput(output) {}

	bool run(const std::wstring& shell, const std::wstring& working_dir, std::vector<std::string> input) noexcept {
		bool result = false;	//assume false		
		
		if (Setup_IO()) {
			if (Create_Shell(shell, working_dir)) {
				std::thread output_reader{ &CProcess::Read_Ouput, this };					
					
				if (Write_Input(input)) {
					if (output_reader.joinable())
						output_reader.join();
					result = Succeeded_by_Return_Code();
				}
			}
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