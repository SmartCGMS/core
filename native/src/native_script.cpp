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

#include "native_script.h"
#include "compiler.h"

#include <rtl/FilesystemLib.h>


CNative_Script::CNative_Script(scgms::IFilter* output) : CBase_Filter(output) {

}

HRESULT CNative_Script::Do_Execute(scgms::UDevice_Event event) {
	HRESULT rc = E_UNEXPECTED;
	
	//Are we interested in this event?
	GUID signal_id = event.signal_id();

	bool desired_event = event.is_level_event();
	size_t sig_index = std::numeric_limits<size_t>::max();
	if (!mSync_To_Any) {
		const auto sig_iter = mSignal_To_Ordinal.find(signal_id);
		desired_event = sig_iter != mSignal_To_Ordinal.end();
		if (desired_event)
			sig_index = std::distance(mSignal_To_Ordinal.begin(), sig_iter);
	}


	if (desired_event) {
		//do we already have this segment?
		const uint64_t segment_id = event.segment_id();
		auto seg_iter = mSegments.find(segment_id);
		if (seg_iter == mSegments.end()) {
			//not yet, we have to insert it
			auto inserted_iter = mSegments.emplace(segment_id, CNative_Segment{mOutput, segment_id, mEntry_Point, mSignal_Ids});
			seg_iter = inserted_iter.first;
			desired_event = inserted_iter.second;			
		}

		if (desired_event) {
			double device_time = event.device_time();
			double level = event.level();
			rc = seg_iter->second.Execute(sig_index, signal_id, device_time, level);

			if ((rc == S_OK) && (signal_id != scgms::signal_Null)) {	//signal_Null is /dev/null
				//the script could have modified the event
				event.device_time() = device_time;
				event.level() = level;
				event.signal_id() = signal_id;
				rc = mOutput.Send(event);
			}	//otherwise, we discard the event and return the code
		}

	} else {
		//on segment stop, remove the segment from memory
		if (event.event_code() == scgms::NDevice_Event_Code::Time_Segment_Stop) 
			mSegments.erase(event.segment_id());


		rc = mOutput.Send(event);
	}


	return rc;
}

HRESULT CNative_Script::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	//Set up the signals and all the common values
	mSignal_Ids[0] = configuration.Read_GUID(rsSynchronization_Signal);
	if ((mSignal_Ids[0] == Invalid_GUID) || (mSignal_Ids[0] == scgms::signal_Null)) {
		error_description.push(L"Synchronization signal cannot be invalid or null id.");
		return E_INVALIDARG;
	}

	mSignal_To_Ordinal[mSignal_Ids[0]] = 0;
	mSync_To_Any = mSignal_Ids[0] == scgms::signal_All;

	for (size_t i = 1; i < mSignal_Ids.size(); i++) {
		const std::wstring res_name = native::rsRequired_Signal_Prefix + std::to_wstring(i + 1);
		const GUID sig_id = configuration.Read_GUID(res_name.c_str());
		mSignal_Ids[i] = sig_id;
		mSignal_To_Ordinal[sig_id] = i;
	}

	//Now, we need to check timestamps of the source file and the produced dll
	filesystem::path script_path = configuration.Read_File_Path(native::rsSource_Filepath);
	//the following two paths must be Read_String to enforce absolute paths, or whatever they desire
	filesystem::path init_path = configuration.Read_String(native::rsEnvironment_Init);
	filesystem::path compiler_path = configuration.Read_String(native::rsCompiler_Name);


	
	if (script_path.empty()) {
		error_description.push(L"Script cannot be empty.");	//because we may still attempt to derive and load dll by its path
		return E_INVALIDARG;
	}

	filesystem::path dll_path = filesystem::path{ script_path }.replace_extension(CDynamic_Library::Default_Extension());

	
	//if there would be no script given, we will try to execute the dll
	bool rebuild = Is_Regular_File_Or_Symlink(script_path) && filesystem::exists(script_path);

	if (rebuild) {
		const auto script_last_write_time = filesystem::last_write_time(script_path);
		
		std::error_code ec;
		const auto dll_last_write_time = filesystem::last_write_time(dll_path, ec);

		rebuild = ec || (script_last_write_time > dll_last_write_time);
		//once builded, we set the last write time for the compiled dll
		//if the dll does not exists, it would throw => ec

		//for building, we need the compiler
		if (compiler_path.empty()) {
			error_description.push(L"Compiler cannot be empty.");	
			return E_INVALIDARG;
		}


		//OK, let's build the dll from the script
		const std::wstring custom_compile_options = configuration.Read_String(native::rsCustom_Compile_Options);
		const std::wstring sdk_include = configuration.Read_String(native::rsSmartCGMS_Include_Dir);
		if (Compile(compiler_path, init_path, script_path, dll_path, sdk_include, custom_compile_options)) {
			//compilation seems to complete OK

			//and set dll's time stamp so that we don't recompile it until next change in the script
			filesystem::last_write_time(dll_path, script_last_write_time);
		} else {
			error_description.push(L"Failed to compile. Please, review the error log file.");
			return E_INVALIDARG;
		}
	}

	//we should have the dll builded, let's load it
	if (!mDll.Load(dll_path)) {
		std::wstring msg = L"Cannot load the compiled script.\nExpected dll path: ";
		msg += dll_path;
		error_description.push(msg);
		return CO_E_ERRORINDLL;
	}
	
	mEntry_Point = static_cast<TNative_Execute>(mDll.Resolve(native::rsScript_Entry_Symbol));
	if (mEntry_Point == nullptr) {
		error_description.push(L"Cannot resolve the entry point of the compiled script!");
		return CO_E_ERRORINDLL;
	}


	return S_OK;
}

