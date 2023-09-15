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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "../../../common/iface/UIIface.h"
#include "../../../common/utils/descriptor_utils.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/rtl/FilesystemLib.h"

#include <vector>
#include <mutex>
#include <fstream>

std::once_flag signals_loaded;
std::vector<scgms::TSignal_Descriptor> custom_signals;
std::vector<std::wstring> signal_strings;
std::vector<std::string> stroke_patterns;

void load_custom_signals() {

	auto path = Get_Application_Dir();
	path /= L"formats";
	path /= L"custom_signals.csv";


	std::wifstream data_file{ path, std::fstream::in };	
	if (data_file.is_open()) {

		std::wstring line;

		// cuts a single column from input line
		auto cut_column = [&line]() -> std::wstring {
			std::wstring retstr{ L"" };

			auto pos = line.find(L';');
			if (pos != std::string::npos) {
				retstr = line.substr(0, pos);
				line.erase(0, pos + 1/*len of ';'*/);
			}
			else retstr = line;

			return trim(retstr);
		};


		while (std::getline(data_file, line)) {
			bool converted_ok = false;

			scgms::TSignal_Descriptor sig_desc = scgms::Null_Signal_Descriptor;
			
			*const_cast<GUID*>(&sig_desc.id) = WString_To_GUID(cut_column(), converted_ok);
			if (!converted_ok) continue;

			signal_strings.push_back(cut_column());
			sig_desc.signal_description = signal_strings[signal_strings.size() - 1].c_str();

			signal_strings.push_back(cut_column());
			sig_desc.unit_description = signal_strings[signal_strings.size() - 1].c_str();


			*const_cast<scgms::NSignal_Unit*>(&sig_desc.unit_id) = static_cast<scgms::NSignal_Unit>(str_2_int(cut_column(), converted_ok));
			if (!converted_ok) continue;
			
			*const_cast<uint32_t*>(&sig_desc.fill_color) = static_cast<uint32_t>(str_2_int(cut_column(), converted_ok));
			if (!converted_ok) continue;
			*const_cast<uint32_t*>(&sig_desc.stroke_color) = static_cast<uint32_t>(str_2_int(cut_column(), converted_ok));
			if (!converted_ok) continue;

			*const_cast<scgms::NSignal_Visualization*>(&sig_desc.visualization) = static_cast<scgms::NSignal_Visualization>(str_2_int(cut_column(), converted_ok));
			if (!converted_ok) continue;

			auto tmp = Narrow_WString(cut_column());
			if (tmp.size() != 1) continue;
			*const_cast<scgms::NSignal_Mark*>(&sig_desc.mark) = static_cast<scgms::NSignal_Mark>(tmp[0]);
			
			tmp = Narrow_WString(cut_column());
			if (!tmp.empty()) {
				stroke_patterns.push_back(tmp);
				sig_desc.stroke_pattern = reinterpret_cast<const scgms::NSignal_Mark*>(stroke_patterns[stroke_patterns.size() - 1].c_str());
			}
			else
				sig_desc.stroke_pattern = nullptr;

			custom_signals.push_back(sig_desc);
		}
	}
}


DLL_EXPORT HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end) {
	std::call_once(signals_loaded, load_custom_signals);
	return do_get_descriptors<scgms::TSignal_Descriptor>(custom_signals, begin, end);
}