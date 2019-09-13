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

#include "persistent_chain_configuration.h"

#include "configuration_link.h"

#include "../../../common/rtl/FilesystemLib.h" 
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/SimpleIni.h" 

#include <fstream>

HRESULT IfaceCalling CPersistent_Chain_Configuration::Load_From_File(const wchar_t *file_path) {
	HRESULT rc = E_UNEXPECTED;

	if (file_path == nullptr) {
		mFile_Path = Get_Application_Dir();
		mFile_Name = rsConfig_File_Name;
		Path_Append(mFile_Path, mFile_Name.c_str());
	}
	else {
		mFile_Path = file_path;
		mFile_Name = file_path;
	}

	std::vector<char> buf;
	std::ifstream configfile;

	try {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
		configfile.open(myconv.to_bytes(mFile_Path));

		if (configfile.is_open()) {
			buf.assign(std::istreambuf_iterator<char>(configfile), std::istreambuf_iterator<char>());
			// fix valgrind's "Conditional jump or move depends on uninitialised value(s)"
			// although we are sending proper length, SimpleIni probably reaches one byte further and reads uninitialized memory
			buf.push_back(0);
			rc = Load_From_Memory(buf.data(), buf.size());
		}

		configfile.close();
	}
	catch (...) {
		rc = E_FAIL;
	}

	return rc;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Load_From_Memory(const char *memory, const size_t len) {
	CSimpleIniW mIni;
	mIni.LoadData(memory, len);

	std::list<CSimpleIniW::Entry> section_names;
	mIni.GetAllSections(section_names);

	// sort by section names - the name would contain zero-padded number, so it is possible to sort it as strings
	section_names.sort([](auto& a, auto& b) {
		return std::wstring(a.pItem).compare(b.pItem) < 0;
	});

	for (auto &section_name : section_names) {
		std::wstring name_str{ section_name.pItem };
		const std::wstring prefix{ rsFilter_Section_Prefix };
		auto res = std::mismatch(prefix.begin(), prefix.end(), name_str.begin());
		if (res.first == prefix.end()) {

			auto uspos = name_str.find(rsFilter_Section_Separator, prefix.size() + 1);
			if (uspos == std::wstring::npos)
				uspos = prefix.size();

			//OK, this is filter section - extract the guid
			const GUID id = WString_To_GUID(std::wstring{ name_str.begin() + uspos + 1, name_str.end() });
			//and get the filter descriptor to load the parameters

			glucose::TFilter_Descriptor desc = glucose::Null_Filter_Descriptor;


			refcnt::SReferenced<glucose::IFilter_Configuration_Link> filter_config{ new CFilter_Configuration_Link{id} };			

			if (glucose::get_filter_descriptor_by_id(id, desc)) {
				//so.. now, try to load the filter parameters - aka filter_config

				for (size_t i = 0; i < desc.parameters_count; i++) {
					
					//does the value exists?
					const wchar_t* str_value = mIni.GetValue(section_name.pItem, desc.config_parameter_name[i]);
					if (str_value) {
						CFilter_Parameter* raw_parameter = new CFilter_Parameter{ desc.parameter_type[i], desc.config_parameter_name[i], str_value };						
						refcnt::SReferenced<glucose::IFilter_Parameter> filter_parameter{ raw_parameter };

						bool valid = true;

						//yes, there is something stored under this key
						switch (filter_parameter.type)
						{
						case glucose::NParameter_Type::ptWChar_Container:
							filter_parameter.wstr = refcnt::WString_To_WChar_Container(str_value);
							break;

						case glucose::NParameter_Type::ptSelect_Time_Segment_ID:
							filter_parameter.select_time_segment_id = WString_To_Select_Time_Segments_Id(str_value);
							break;

						case glucose::NParameter_Type::ptRatTime:
						case glucose::NParameter_Type::ptDouble:
							filter_parameter.dbl = mIni.GetDoubleValue(section_name.pItem, desc.config_parameter_name[i]);
							break;

						case glucose::NParameter_Type::ptInt64:
						case glucose::NParameter_Type::ptSubject_Id:
							filter_parameter.int64 = mIni.GetLongValue(section_name.pItem, desc.config_parameter_name[i]);
							break;

						case glucose::NParameter_Type::ptBool:
							filter_parameter.boolean = mIni.GetBoolValue(section_name.pItem, desc.config_parameter_name[i]);
							break;

						case glucose::NParameter_Type::ptModel_Id:
						case glucose::NParameter_Type::ptMetric_Id:
						case glucose::NParameter_Type::ptModel_Signal_Id:
						case glucose::NParameter_Type::ptSignal_Id:
						case glucose::NParameter_Type::ptSolver_Id:
						case glucose::NParameter_Type::ptDevice_Driver_Id:
							filter_parameter.guid = WString_To_GUID(str_value);
							break;

						case glucose::NParameter_Type::ptModel_Bounds:
							filter_parameter.parameters = WString_To_Model_Parameters(str_value);
							break;

						default:
							valid = false;
						}

						if (valid) {
							//create the parameter name container, only if it is really neded - it is reference counted!
							filter_parameter.config_name = refcnt::WString_To_WChar_Container(desc.config_parameter_name[i]);
							filter_config.push_back(filter_parameter);	//overriden to call add ref!
						}

						Release_Filter_Parameter(filter_parameter);	//do not forget, it is reference counted and we could have created some!
					}
				}

				//and finally, add the new link into the filter chain
				new_chain.push_back({ desc, filter_config });
			}
		}
	}
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Save_To_File(const wchar_t *file_path) {
	CSimpleIniW mIni;
}