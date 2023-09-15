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

#include "persistent_chain_configuration.h"
#include "configuration_link.h"
#include "filters.h"

#include <scgms/rtl/FilesystemLib.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/manufactory.h>
#include <scgms/rtl/rattime.h>
#include <scgms/lang/dstrings.h>
#include <scgms/utils/SimpleIni.h>
#include <scgms/utils/string_utils.h>

#include <fstream>
#include <exception>

CPersistent_Chain_Configuration::CPersistent_Chain_Configuration() {
	//
}

CPersistent_Chain_Configuration::~CPersistent_Chain_Configuration() noexcept {
	//
}

std::wstring CPersistent_Chain_Configuration::Get_Parent_Path() noexcept {
	std::wstring parent_path = mFile_Path.empty() ? Get_Dll_Dir().wstring() : mFile_Path.parent_path().wstring();
	return parent_path;
}

void CPersistent_Chain_Configuration::Advertise_Parent_Path() noexcept {
	std::wstring parent_path = Get_Parent_Path();
	
	for (scgms::IFilter_Configuration_Link* link : mData) {
		link->Set_Parent_Path(parent_path.c_str());
	}
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Load_From_File(const wchar_t* file_path, refcnt::wstr_list* error_description) noexcept {	
	mFile_Path.clear();
	HRESULT rc = E_UNEXPECTED;

	if ((file_path == nullptr) || (*file_path == 0))
		return E_INVALIDARG;
	
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);

	std::error_code ec;
	filesystem::path working_file_path = filesystem::absolute(std::wstring{ file_path }, ec);
	if (ec) {                
        shared_error_description.push(Widen_String(ec.message()));
		return E_INVALIDARG;
	}		
	 	
	std::ifstream configfile;
	configfile.open(working_file_path);

	if (configfile.is_open()) {
		mFile_Path = working_file_path;

		std::vector<char> buf;
		buf.assign(std::istreambuf_iterator<char>(configfile), std::istreambuf_iterator<char>());
		// fix valgrind's "Conditional jump or move depends on uninitialised value(s)"
		// although we are sending proper length, SimpleIni probably reaches one byte further and reads uninitialized memory
		buf.push_back(0);
		rc = Load_From_Memory(buf.data(), buf.size(), error_description);	//also clears mFile_Path!
		if (Succeeded(rc)) {			
			Advertise_Parent_Path();
		}

		configfile.close();
	}
	else
		rc = ERROR_FILE_NOT_FOUND;

	
	return rc;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Load_From_Memory(const char* memory, const size_t len, refcnt::wstr_list* error_description) noexcept {
	CSimpleIniW mIni;

	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);

	if (mIni.LoadData(memory, len) != SI_Error::SI_OK) {
		shared_error_description.push(L"Could not load INI file from memory");
		return E_FAIL;
	}

	bool loaded_all_filters = true;
	bool encountered_E_NOT_SET = false;

	const std::wstring parent_path = Get_Parent_Path();


	std::list<CSimpleIniW::Entry> section_names;
	mIni.GetAllSections(section_names);

	// sort by section names - the name would contain zero-padded number, so it is possible to sort it as strings
	section_names.sort([](auto& a, auto& b) {
		return std::wstring(a.pItem).compare(b.pItem) < 0;
		});

	for (auto& section_name : section_names) {
		std::wstring name_str{ section_name.pItem };
		const std::wstring prefix{ rsFilter_Section_Prefix };
		auto res = std::mismatch(prefix.begin(), prefix.end(), name_str.begin());
		if (res.first == prefix.end()) {

			auto uspos = name_str.find(rsFilter_Section_Separator, prefix.size() + 1);
			if (uspos == std::wstring::npos)
				uspos = prefix.size();

			//OK, this is filter section - extract the guid
			const std::wstring section_id_str { name_str.begin() + uspos + 1, name_str.end() };
			bool section_id_ok;
			const GUID id = WString_To_GUID(section_id_str, section_id_ok);
			//and get the filter descriptor to load the parameters

			scgms::TFilter_Descriptor desc = scgms::Null_Filter_Descriptor;

				

			if (section_id_ok && scgms::get_filter_descriptor_by_id(id, desc)) {
				refcnt::SReferenced<scgms::IFilter_Configuration_Link> filter_config{ new CFilter_Configuration_Link{id} };

				//so.. now, try to load the filter parameters - aka filter_config
				if (filter_config) {
					for (size_t i = 0; i < desc.parameters_count; i++) {

						//does the value exists?
						const wchar_t* str_value = mIni.GetValue(section_name.pItem, desc.config_parameter_name[i]);
						if (str_value) {

							std::unique_ptr<CFilter_Parameter> raw_filter_parameter = std::make_unique<CFilter_Parameter>(desc.parameter_type[i], desc.config_parameter_name[i]);
							raw_filter_parameter->Set_Parent_Path(parent_path.c_str()); //needed to make the deferred parameters work
							const HRESULT valid_rc = raw_filter_parameter->from_string(desc.parameter_type[i], str_value);							

							if (Succeeded(valid_rc) || (valid_rc == E_NOT_SET)) {
								scgms::IFilter_Parameter* raw_param = static_cast<scgms::IFilter_Parameter*>(raw_filter_parameter.get());								

								if (Succeeded(filter_config->add(&raw_param, &raw_param + 1)))
									raw_filter_parameter.release();

								if (valid_rc == E_NOT_SET) {
									encountered_E_NOT_SET = true;
									std::wstring error_desc = dsVar_Not_Set_Filter_Parameter_Value;
									error_desc.append(desc.description);
									error_desc.append(L" (2)");
									error_desc.append(desc.ui_parameter_name[i]);
									error_desc.append(L" (3)");
									error_desc.append(str_value);
									shared_error_description.push(error_desc.c_str());
								}
							}
							else {
								std::wstring error_desc = dsMalformed_Filter_Parameter_Value;
								error_desc.append(desc.description);
								error_desc.append(L" (2)");
								error_desc.append(desc.ui_parameter_name[i]);
								error_desc.append(L" (3)");
								error_desc.append(str_value);
								shared_error_description.push(error_desc.c_str());
							}

						}
						else if (desc.parameter_type[i] != scgms::NParameter_Type::ptNull) {
							//this parameter is not configured, warn about it
							std::wstring error_desc = dsFilter_Parameter_Not_Configured;
							error_desc.append(desc.description);
							error_desc.append(L" (2)");
							error_desc.append(desc.ui_parameter_name[i]);
							shared_error_description.push(error_desc.c_str());
						}
					}


					//and finally, add the new link into the filter chain
					{
						auto raw_filter_config = filter_config.get();
						add(&raw_filter_config, &raw_filter_config + 1);
					}
				}
				else {
					//memory failure
					shared_error_description.push(rsFailed_to_allocate_memory);
					return E_FAIL;
				}
			}
			else {					
				loaded_all_filters = false;
				std::wstring error_desc = dsCannot_Resolve_Filter_Descriptor + section_id_str;
				shared_error_description.push(error_desc.c_str());
			}
		}
		else {
			std::wstring error_desc = dsInvalid_Section_Name + name_str;
			shared_error_description.push(error_desc.c_str());
		}

	}

	Advertise_Parent_Path();	//called once more to ensure that everybody got the parent path

	if (!loaded_all_filters)
		describe_loaded_filters(shared_error_description);

	return (loaded_all_filters || encountered_E_NOT_SET) ? S_OK : S_FALSE;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Save_To_File(const wchar_t* file_path, refcnt::wstr_list* error_description) noexcept {
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);

	//1. determine the file path to save to
	const bool empty_file_path = (file_path == nullptr) || (*file_path == 0);

	if (empty_file_path && (mFile_Path.empty())) return E_ILLEGAL_METHOD_CALL;

	filesystem::path working_file_path;
	if (empty_file_path) working_file_path = mFile_Path;
	else {
		std::error_code ec;
		working_file_path = filesystem::absolute(std::wstring{ file_path }, ec);
		if (ec) {
			refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);
                        shared_error_description.push(Widen_String(ec.message()));
			return E_INVALIDARG;
		}		
	}
	

	//2. produce the ini file content
	CSimpleIniW ini;
	uint32_t section_counter = 1;
	scgms::CSignal_Description signal_descriptors;	//make the GUID in the ini human-readable by attaching comments

	auto ini_SetValue = [&ini](const std::wstring &a_pSection,
		const wchar_t* a_pKey,
		const std::wstring& a_pValue,
		const std::wstring& a_pComment = std::wstring{},
		bool            a_bForceReplace = false) {

			ini.SetValue(a_pSection.c_str(), a_pKey, a_pValue.empty() ? nullptr : a_pValue.c_str(), a_pComment.empty() ? nullptr : a_pComment.c_str(), a_bForceReplace);
	};


	scgms::IFilter_Configuration_Link** filter_begin, ** filter_end;
	HRESULT rc = get(&filter_begin, &filter_end);
	if (rc != S_OK) return rc;
	for (; filter_begin != filter_end; filter_begin++) {
		auto filter = *filter_begin;
		GUID filter_id;
		rc = filter->Get_Filter_Id(&filter_id);
		if (rc != S_OK) return rc;

		const std::wstring id_str = std::wstring(rsFilter_Section_Prefix) + rsFilter_Section_Separator + Get_Padded_Number(section_counter++, 3) + rsFilter_Section_Separator + GUID_To_WString(filter_id);
		auto section = ini.GetSection(id_str.c_str());
		if (!section) {
			//if the section does not exist yet, create it by writing a comment there - the filter description
			scgms::TFilter_Descriptor filter_desc = scgms::Null_Filter_Descriptor;
                            if (scgms::get_filter_descriptor_by_id(filter_id, filter_desc)) {                                    
								ini_SetValue(id_str.c_str(), nullptr, std::wstring{}, std::wstring{ rsIni_Comment_Prefix }.append(filter_desc.description));
                            }
		}

		scgms::IFilter_Parameter** parameter_begin, ** parameter_end;
		rc = filter->get(&parameter_begin, &parameter_end);
		if (!Succeeded(rc)) return rc;	//rc may be also S_FALSE if no parameter has been set yet


		for (; parameter_begin != parameter_end; parameter_begin++) {
			scgms::SFilter_Parameter param_shared = refcnt::make_shared_reference_ext<scgms::SFilter_Parameter, scgms::IFilter_Parameter>(*parameter_begin, true);

			const wchar_t* config_name = param_shared.configuration_name();
			if (!config_name) 
				return E_UNEXPECTED;


			//let the filter parameter to convert its value to the string				
			std::wstring converted = param_shared.as_wstring(rc, false);

			if (rc == E_NOT_SET) {
				std::wstring error_desc = dsVar_Not_Set_Filter_Parameter_Value;
				error_desc.append(id_str);
				error_desc.append(L" (2)");
				error_desc.append(config_name);
				error_desc.append(L" (3)");
				error_desc.append(converted);
				shared_error_description.push(error_desc.c_str());
			} else 
				if (rc != S_OK) 
					return rc;
				
			std::wstring comment = L"";

			//resolve any possible comment
			const auto param_type = param_shared.type();
			if (param_type == scgms::NParameter_Type::ptInvalid)
				return E_UNEXPECTED;

			switch (param_type) {
				case scgms::NParameter_Type::ptSignal_Model_Id:
				case scgms::NParameter_Type::ptDiscrete_Model_Id:
				case scgms::NParameter_Type::ptMetric_Id:
				case scgms::NParameter_Type::ptModel_Produced_Signal_Id:
				case scgms::NParameter_Type::ptSignal_Id:
				case scgms::NParameter_Type::ptSolver_Id:
				{
					const GUID val = param_shared.as_guid(rc);
					if (rc != S_OK) return rc;

					wchar_t* id_desc_ptr = Describe_GUID(val, param_shared.type(), signal_descriptors);
					if (id_desc_ptr) {
						comment = L"; ";
						comment += id_desc_ptr;							
					}
											
				}
				break;

				default: break;
			} //switch param_type


			//and, write the value				
			ini_SetValue(id_str, config_name, converted, comment);
		}
	}


	std::string content;
	rc = ini.Save(content) == SI_Error::SI_OK ? S_OK : E_FAIL;

	//3. save the content
	if (rc == S_OK) {
		std::ofstream config_file(working_file_path, std::ofstream::binary);
		if (config_file.is_open()) {
			config_file << content;
			config_file.close();

			if (!empty_file_path) {
				mFile_Path = std::move(working_file_path);
				Advertise_Parent_Path();
			}
		}
	}

	return S_OK;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::add(scgms::IFilter_Configuration_Link** begin, scgms::IFilter_Configuration_Link** end) noexcept {
	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Configuration_Link*>::add(begin, end);
	if (rc == S_OK)
		Advertise_Parent_Path();

	return rc;	
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Get_Parent_Path(refcnt::wstr_container** path) noexcept {
	const std::wstring path_to_return = mFile_Path.empty() ? Get_Dll_Dir().wstring() : mFile_Path.parent_path().wstring();
	*path = refcnt::WString_To_WChar_Container(path_to_return.c_str());
	return S_OK;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Set_Parent_Path(const wchar_t* parent_path) noexcept {
	if (!parent_path || (*parent_path == 0)) return E_INVALIDARG;

	HRESULT rc = S_OK;
	mFile_Path = parent_path;

	if (Is_Directory(mFile_Path))
		mFile_Path.append(".");

	for (scgms::IFilter_Configuration_Link* link : mData) {
		if (!Succeeded(link->Set_Parent_Path(parent_path)))
			rc = E_UNEXPECTED;
	}

	return rc;
}



HRESULT IfaceCalling CPersistent_Chain_Configuration::Set_Variable(const wchar_t* name, const wchar_t* value) noexcept {
	if (!name || (*name == 0)) return E_INVALIDARG;
	if (name == CFilter_Parameter::mUnused_Variable_Name) return TYPE_E_AMBIGUOUSNAME;

	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Configuration_Link*>::empty();
	if (rc == S_FALSE) {
		for (scgms::IFilter_Configuration_Link* link : mData) {
			if (!Succeeded(link->Set_Variable(name, value)))
				rc = E_UNEXPECTED;
		}
	}

	return rc;
}


wchar_t* CPersistent_Chain_Configuration::Describe_GUID(const GUID& val, const scgms::NParameter_Type param_type, const scgms::CSignal_Description& signal_descriptors) const noexcept {
	wchar_t* result = nullptr;

	switch (param_type) {

	case scgms::NParameter_Type::ptModel_Produced_Signal_Id:
	case scgms::NParameter_Type::ptSignal_Id:
	{
		scgms::TSignal_Descriptor sig_desc = scgms::Null_Signal_Descriptor;
		if (signal_descriptors.Get_Descriptor(val, sig_desc))
			result = const_cast<wchar_t*>(sig_desc.signal_description);
	}
	break;

	case scgms::NParameter_Type::ptSignal_Model_Id:
	case scgms::NParameter_Type::ptDiscrete_Model_Id: {
		{
			scgms::TModel_Descriptor model_desc = scgms::Null_Model_Descriptor;
			if (scgms::get_model_descriptor_by_id(val, model_desc))
				result = const_cast<wchar_t*>(model_desc.description);
		}
		break;
	}

	case scgms::NParameter_Type::ptMetric_Id:
	{
		scgms::TMetric_Descriptor metric_desc = scgms::Null_Metric_Descriptor;
		if (scgms::get_metric_descriptor_by_id(val, metric_desc))
			result = const_cast<wchar_t*>(metric_desc.description);
	}
	break;

	case scgms::NParameter_Type::ptSolver_Id:
	{
		scgms::TSolver_Descriptor solver_desc = scgms::Null_Solver_Descriptor;
		if (scgms::get_solver_descriptor_by_id(val, solver_desc))
			result = const_cast<wchar_t*>(solver_desc.description);
	}
	break;


	default: result = nullptr; break;
	}

	return result;
}

DLL_EXPORT HRESULT IfaceCalling create_persistent_filter_chain_configuration(scgms::IPersistent_Filter_Chain_Configuration** configuration) noexcept {
	return Manufacture_Object<CPersistent_Chain_Configuration, scgms::IPersistent_Filter_Chain_Configuration>(configuration);
}
