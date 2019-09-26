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
#include "../../../common/rtl/manufactory.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/SimpleIni.h" 

#include <fstream>

HRESULT IfaceCalling CPersistent_Chain_Configuration::Load_From_File(const wchar_t *file_path) {
	HRESULT rc = E_UNEXPECTED;

	if ((file_path == nullptr) || (*file_path == 0)) {
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
	bool loaded_all_filters = true;

	try {
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
							refcnt::SReferenced<glucose::IFilter_Parameter> filter_parameter{ new CFilter_Parameter{ desc.parameter_type[i], desc.config_parameter_name[i] } };

							bool valid = true;

							//yes, there is something stored under this key
							switch (desc.parameter_type[i]) {

							case glucose::NParameter_Type::ptWChar_Container:
								{
									refcnt::wstr_container *wstr = refcnt::WString_To_WChar_Container(str_value);
									valid = filter_parameter->Set_WChar_Container(wstr) == S_OK;
									wstr->Release();
								}
								break;

							case glucose::NParameter_Type::ptSelect_Time_Segment_ID:
								{
									glucose::time_segment_id_container *ids = WString_To_Select_Time_Segments_Id(str_value);
									valid = filter_parameter->Set_Time_Segment_Id_Container(ids) == S_OK;
									ids->Release();
								}
								break;

							case glucose::NParameter_Type::ptRatTime:
							case glucose::NParameter_Type::ptDouble:
								valid = filter_parameter->Set_Double(mIni.GetDoubleValue(section_name.pItem, desc.config_parameter_name[i])) == S_OK;
								break;

							case glucose::NParameter_Type::ptInt64:
							case glucose::NParameter_Type::ptSubject_Id:
								valid = filter_parameter->Set_Int64(mIni.GetLongValue(section_name.pItem, desc.config_parameter_name[i])) == S_OK;
								break;

							case glucose::NParameter_Type::ptBool:
								valid = filter_parameter->Set_Bool(mIni.GetBoolValue(section_name.pItem, desc.config_parameter_name[i]) ? 0 : static_cast<uint8_t>(-1)) == S_OK;
								break;

							case glucose::NParameter_Type::ptModel_Id:
							case glucose::NParameter_Type::ptMetric_Id:
							case glucose::NParameter_Type::ptModel_Signal_Id:
							case glucose::NParameter_Type::ptSignal_Id:
							case glucose::NParameter_Type::ptSolver_Id:
							case glucose::NParameter_Type::ptDevice_Driver_Id:
								{
									const GUID tmp_guid = WString_To_GUID(str_value);
									valid = filter_parameter->Set_GUID(&tmp_guid) == S_OK;
								}
								break;

							case glucose::NParameter_Type::ptModel_Bounds:
								{
									glucose::IModel_Parameter_Vector *parameters = WString_To_Model_Parameters(str_value);
									valid = filter_parameter->Set_Model_Parameters(parameters) == S_OK;
									parameters->Release();
								}
								break;

							default:
								valid = false;
							} //switch (desc.parameter_type[i])	{

							if (valid) {
								auto raw_param = filter_parameter.get();
								filter_config->add(&raw_param, &raw_param + 1);
							}

						}
					}

					//and finally, add the new link into the filter chain
					{
						auto raw_filter_config = filter_config.get();
						add(&raw_filter_config, &raw_filter_config + 1);						
					}
				}
				else
					loaded_all_filters = false;
			}
		}
	}
	catch (...) {
		return E_FAIL;
	}

	return loaded_all_filters ? S_OK : S_FALSE;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Save_To_File(const wchar_t *file_path) {
	CSimpleIniW ini;
	uint32_t section_counter = 1;

	try {
		glucose::IFilter_Configuration_Link **filter_begin, **filter_end;		
		HRESULT rc = get(&filter_begin, &filter_end);
		if (rc != S_OK) return rc;
		for (auto filter=*filter_begin; filter != *filter_end; filter++) {
			GUID filter_id;
			rc = filter->Get_Filter_Id(&filter_id);
			if (rc != S_OK) return rc;

			const std::wstring id_str = std::wstring(rsFilter_Section_Prefix) + rsFilter_Section_Separator + Get_Padded_Number(section_counter++, 3) + rsFilter_Section_Separator + GUID_To_WString(filter_id);
			auto section = ini.GetSection(id_str.c_str());
			if (!section) {
				//if the section does not exist yet, create it by writing a comment there - the filter description
				glucose::TFilter_Descriptor filter_desc = glucose::Null_Filter_Descriptor;
				if (glucose::get_filter_descriptor_by_id(filter_id, filter_desc))
					ini.SetValue(id_str.c_str(), nullptr, nullptr, std::wstring{ rsIni_Comment_Prefix }.append(filter_desc.description).c_str());
			}

			glucose::IFilter_Parameter **parameter_begin, **parameter_end;
			rc = filter->get(&parameter_begin, &parameter_end);
			if (rc != S_OK) return rc;

			for (auto parameter = *parameter_begin; parameter != *parameter_end; parameter++) {
				glucose::NParameter_Type param_type;
				rc = parameter->Get_Type(&param_type);
				if (rc != S_OK) return rc;

				wchar_t *config_name;
				rc = parameter->Get_Config_Name(&config_name);
				if (rc != S_OK) return rc;


				switch (param_type) {
				case glucose::NParameter_Type::ptWChar_Container:

					refcnt::wstr_container *wstr;
					rc = parameter->Get_WChar_Container(&wstr);
					if (rc != S_OK) return rc;

					ini.SetValue(id_str.c_str(), config_name, WChar_Container_To_WString(wstr).c_str());
					wstr->Release();
					break;

				case glucose::NParameter_Type::ptSelect_Time_Segment_ID:
					glucose::time_segment_id_container *ids;
					rc = parameter->Get_Time_Segment_Id_Container(&ids);
					if (rc != S_OK) return rc;

					ini.SetValue(id_str.c_str(), config_name, Select_Time_Segments_Id_To_WString(ids).c_str());
					ids->Release();
					break;

				case glucose::NParameter_Type::ptRatTime:
				case glucose::NParameter_Type::ptDouble:
				{
					double val;
					rc = parameter->Get_Double(&val);
					if (rc != S_OK) return rc;

					ini.SetDoubleValue(id_str.c_str(), config_name, val);
				}
				break;

				case glucose::NParameter_Type::ptInt64:
				case glucose::NParameter_Type::ptSubject_Id:
				{
					int64_t val;
					rc = parameter->Get_Int64(&val);
					if (rc != S_OK) return rc;
					ini.SetLongValue(id_str.c_str(), config_name, static_cast<long>(val));
				}
				break;

				case glucose::NParameter_Type::ptBool:
				{
					uint8_t val;
					rc = parameter->Get_Bool(&val);
					if (rc != S_OK) return rc;
					ini.SetBoolValue(id_str.c_str(), config_name, val != 0);
				}
				break;

				case glucose::NParameter_Type::ptModel_Id:
				case glucose::NParameter_Type::ptMetric_Id:
				case glucose::NParameter_Type::ptModel_Signal_Id:
				case glucose::NParameter_Type::ptSignal_Id:
				case glucose::NParameter_Type::ptSolver_Id:
				case glucose::NParameter_Type::ptDevice_Driver_Id:
				{
					GUID val;
					rc = parameter->Get_GUID(&val);
					if (rc != S_OK) return rc;

					ini.SetValue(id_str.c_str(), config_name, GUID_To_WString(val).c_str());
				}
				break;

				case glucose::NParameter_Type::ptModel_Bounds:
					glucose::IModel_Parameter_Vector *model_parameters;
					rc = parameter->Get_Model_Parameters(&model_parameters);
					if (rc != S_OK) return rc;

					ini.SetValue(id_str.c_str(), config_name, Model_Parameters_To_WString(model_parameters).c_str());
					model_parameters->Release();
					break;

				default: break;
				} //switch (param_type) {
			}
		}


		std::string content;
		ini.Save(content);
		std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
		std::ofstream config_file(myconv.to_bytes(mFile_Path));
		if (config_file.is_open()) {
			config_file << content;
			config_file.close();
		}
	}
	catch (...) {
		return E_FAIL;
	}

	return S_OK;
}

HRESULT IfaceCalling create_persistent_filter_chain_configuration(glucose::IPersistent_Filter_Chain_Configuration **configuration) {
	return Manufacture_Object<CPersistent_Chain_Configuration, glucose::IPersistent_Filter_Chain_Configuration>(configuration);
}