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

#include "persistent_chain_configuration.h"
#include "configuration_link.h"
#include "filters.h"

#include "../../../common/rtl/FilesystemLib.h" 
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/SimpleIni.h" 
#include "../../../common/utils/string_utils.h" 

#include <fstream>
#include <exception>

void CPersistent_Chain_Configuration::Advertise_Parent_Path() {
	std::wstring parent_path = mFile_Path.empty() ? Get_Dll_Dir() : mFile_Path.parent_path().wstring();
	
	for (scgms::IFilter_Configuration_Link* link : *this) {
		link->Set_Parent_Path(parent_path.c_str());
	}
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Load_From_File(const wchar_t* file_path, refcnt::wstr_list* error_description) {	
	mFile_Path.clear();
	HRESULT rc = E_UNEXPECTED;

	if ((file_path == nullptr) || (*file_path == 0))
		return E_INVALIDARG;
	
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);

	std::error_code ec;
	filesystem::path working_file_path = filesystem::absolute(std::wstring{ file_path }, ec);
	if (ec) {
		shared_error_description.push(Widen_Char(ec.message().c_str()));
		return E_INVALIDARG;
	}	
	
	
	try {		
		std::ifstream configfile;
		configfile.open(working_file_path);

		if (configfile.is_open()) {
			std::vector<char> buf;
			buf.assign(std::istreambuf_iterator<char>(configfile), std::istreambuf_iterator<char>());
			// fix valgrind's "Conditional jump or move depends on uninitialised value(s)"
			// although we are sending proper length, SimpleIni probably reaches one byte further and reads uninitialized memory
			buf.push_back(0);
			rc = Load_From_Memory(buf.data(), buf.size(), error_description);	//also clears mFile_Path!
			if (Succeeded(rc)) {
				mFile_Path = working_file_path;	//so that we clearly sets new one whehn we succeed
				Advertise_Parent_Path();
			}

			configfile.close();
		}
		else
			rc = ERROR_FILE_NOT_FOUND;

		
	}
	catch (const std::exception & ex) {
		// specific handling for all exceptions extending std::exception, except
		// std::runtime_error which is handled explicitly
		std::wstring error_desc = Widen_Char(ex.what());
		shared_error_description.push(error_desc);
		
		return E_FAIL;
	}
	catch (...) {
		rc = E_FAIL;
	}

	return rc;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Load_From_Memory(const char* memory, const size_t len, refcnt::wstr_list* error_description) {
	CSimpleIniW mIni;
	bool loaded_all_filters = true;
	refcnt::Swstr_list shared_error_description = refcnt::make_shared_reference_ext<refcnt::Swstr_list, refcnt::wstr_list>(error_description, true);

	try {
		mFile_Path = Get_Dll_Dir();

		mIni.LoadData(memory, len);

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

					for (size_t i = 0; i < desc.parameters_count; i++) {

						//does the value exists?
						const wchar_t* str_value = mIni.GetValue(section_name.pItem, desc.config_parameter_name[i]);
						if (str_value) {
							scgms::SFilter_Parameter filter_parameter;
							{
								std::unique_ptr<CFilter_Parameter> raw_filter_parameter = std::make_unique<CFilter_Parameter>(desc.parameter_type[i], desc.config_parameter_name[i]);
								filter_parameter = refcnt::make_shared_reference_ext<scgms::SFilter_Parameter, scgms::IFilter_Parameter>(raw_filter_parameter.get(), true);
								raw_filter_parameter.release();
							}

							bool valid = false;

							//yes, there is something stored under this key
							switch (desc.parameter_type[i]) {

							case scgms::NParameter_Type::ptWChar_Array:
								valid = filter_parameter.set_wstring(str_value) == S_OK;
								break;

							case scgms::NParameter_Type::ptInt64_Array:
								valid = filter_parameter.int_array_from_wstring(str_value) == S_OK;
								break;

							case scgms::NParameter_Type::ptRatTime:
							case scgms::NParameter_Type::ptDouble:
							{
								double val = wstr_2_dbl(str_value, valid);
								if (valid)
									valid = filter_parameter->Set_Double(val) == S_OK;
							}
							break;

							case scgms::NParameter_Type::ptInt64:
							case scgms::NParameter_Type::ptSubject_Id:
							{
								int64_t val = wstr_2_int(str_value, valid);
								if (valid)
									valid = filter_parameter->Set_Int64(val) == S_OK;
							}
							break;

							case scgms::NParameter_Type::ptBool:
								valid = filter_parameter.set_bool(mIni.GetBoolValue(section_name.pItem, desc.config_parameter_name[i])) == S_OK;
								break;

							case scgms::NParameter_Type::ptSignal_Model_Id:
							case scgms::NParameter_Type::ptDiscrete_Model_Id:
							case scgms::NParameter_Type::ptMetric_Id:
							case scgms::NParameter_Type::ptModel_Produced_Signal_Id:
							case scgms::NParameter_Type::ptSignal_Id:
							case scgms::NParameter_Type::ptSolver_Id:
							{								
								const GUID tmp_guid = WString_To_GUID(str_value, valid);								
								if (valid)
									valid = filter_parameter.set_GUID(tmp_guid) == S_OK;
							}
							break;

							case scgms::NParameter_Type::ptDouble_Array:
								valid = filter_parameter.double_array_from_wstring(str_value) == S_OK;
								break;

							default:
								valid = false;
							} //switch (desc.parameter_type[i])	{

							if (valid) {
								auto raw_param = filter_parameter.get();
								filter_config->add(&raw_param, &raw_param + 1);
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

		Advertise_Parent_Path();
	}
	catch (const std::exception & ex) {
		// specific handling for all exceptions extending std::exception, except
		// std::runtime_error which is handled explicitly
		std::wstring error_desc = Widen_Char(ex.what());
		shared_error_description.push(error_desc.c_str());
		return E_FAIL;
	}
	catch (...) {
		return E_FAIL;
	}

	if (!loaded_all_filters)
		describe_loaded_filters(shared_error_description);

	return loaded_all_filters ? S_OK : S_FALSE;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Save_To_File(const wchar_t* file_path, refcnt::wstr_list* error_description) {
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
			shared_error_description.push(Widen_Char(ec.message().c_str()));
			return E_INVALIDARG;
		}		
	}
	

	//2. produce the ini file content
	CSimpleIniW ini;
	uint32_t section_counter = 1;
	scgms::CSignal_Description signal_descriptors;	//make the GUID in the ini human-readable by attaching comments

	try {
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
				if (scgms::get_filter_descriptor_by_id(filter_id, filter_desc))
					ini.SetValue(id_str.c_str(), nullptr, nullptr, std::wstring{ rsIni_Comment_Prefix }.append(filter_desc.description).c_str());
			}

			scgms::IFilter_Parameter** parameter_begin, ** parameter_end;
			rc = filter->get(&parameter_begin, &parameter_end);
			if (!Succeeded(rc)) return rc;	//rc may be also S_FALSE if no parameter has been set yet

			for (; parameter_begin != parameter_end; parameter_begin++) {
				auto parameter = *parameter_begin;
				scgms::NParameter_Type param_type;
				rc = parameter->Get_Type(&param_type);
				if (rc != S_OK) return rc;

				wchar_t* config_name;
				rc = parameter->Get_Config_Name(&config_name);
				if (rc != S_OK) return rc;


				switch (param_type) {
					case scgms::NParameter_Type::ptWChar_Array:

						refcnt::wstr_container* wstr;
						rc = parameter->Get_WChar_Container(&wstr, false);
						if (rc != S_OK) return rc;

						ini.SetValue(id_str.c_str(), config_name, WChar_Container_To_WString(wstr).c_str());
						wstr->Release();
						break;

					case scgms::NParameter_Type::ptInt64_Array:
						scgms::time_segment_id_container* ids;
						rc = parameter->Get_Time_Segment_Id_Container(&ids);
						if (rc != S_OK) return rc;

						ini.SetValue(id_str.c_str(), config_name, Select_Time_Segments_Id_To_WString(ids).c_str());
						ids->Release();
						break;

					case scgms::NParameter_Type::ptRatTime:
					case scgms::NParameter_Type::ptDouble:
					{
						double val;
						rc = parameter->Get_Double(&val);
						if (rc != S_OK) return rc;

						std::wstring time_str;
						
						if (param_type == scgms::NParameter_Type::ptRatTime) {
							time_str = Rat_Time_To_Default_WStr(val);
							if (!time_str.empty()) time_str = L"; " + time_str;
						}

						ini.SetValue(id_str.c_str(), config_name, dbl_2_wstr(val).c_str(),  //dbl_2_wstr is more precise than ini's SetDoubleValue
										time_str.empty() ? nullptr : time_str.c_str());	
					}
					break;

					case scgms::NParameter_Type::ptInt64:
					case scgms::NParameter_Type::ptSubject_Id:
					{
						int64_t val;
						rc = parameter->Get_Int64(&val);
						if (rc != S_OK) return rc;
						ini.SetLongValue(id_str.c_str(), config_name, static_cast<long>(val));
					}
					break;

					case scgms::NParameter_Type::ptBool:
					{
						BOOL val;
						rc = parameter->Get_Bool(&val);
						if (rc != S_OK) return rc;
						ini.SetBoolValue(id_str.c_str(), config_name, val != FALSE);
					}
					break;

					case scgms::NParameter_Type::ptSignal_Model_Id:
					case scgms::NParameter_Type::ptDiscrete_Model_Id:
					case scgms::NParameter_Type::ptMetric_Id:
					case scgms::NParameter_Type::ptModel_Produced_Signal_Id:
					case scgms::NParameter_Type::ptSignal_Id:
					case scgms::NParameter_Type::ptSolver_Id:
					{
						GUID val;
						rc = parameter->Get_GUID(&val);
						if (rc != S_OK) return rc;

						wchar_t* id_desc_ptr = Describe_GUID(val, param_type, signal_descriptors);
						std::wstring commented_comment = L"; ";
						if (id_desc_ptr) {
							commented_comment += id_desc_ptr;
							id_desc_ptr = const_cast<wchar_t*>(commented_comment.c_str());
						}
						
						ini.SetValue(id_str.c_str(), config_name, GUID_To_WString(val).c_str(), id_desc_ptr);
					}
					break;

					case scgms::NParameter_Type::ptDouble_Array:
						scgms::IModel_Parameter_Vector* model_parameters;
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

		//3. save the content
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
	catch (...) {
		return E_FAIL;
	}

	return S_OK;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::add(scgms::IFilter_Configuration_Link** begin, scgms::IFilter_Configuration_Link** end) {
	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Configuration_Link*>::add(begin, end);
	if (rc == S_OK)
		Advertise_Parent_Path();

	return rc;	
}

HRESULT IfaceCalling create_persistent_filter_chain_configuration(scgms::IPersistent_Filter_Chain_Configuration** configuration) {
	return Manufacture_Object<CPersistent_Chain_Configuration, scgms::IPersistent_Filter_Chain_Configuration>(configuration);
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Get_Parent_Path(refcnt::wstr_container** path) {
	const std::wstring path_to_return = mFile_Path.empty() ? Get_Dll_Dir() : mFile_Path.parent_path().wstring();
	*path = refcnt::WString_To_WChar_Container(path_to_return.c_str());
	return S_OK;
}

HRESULT IfaceCalling CPersistent_Chain_Configuration::Set_Parent_Path(const wchar_t* parent_path) {
	if (!parent_path || (*parent_path == 0)) return E_INVALIDARG;

	HRESULT rc = S_OK;
	mFile_Path = parent_path;

	if (Is_Directory(parent_path))
		mFile_Path /= ".";

	for (scgms::IFilter_Configuration_Link* link : *this) {
		if (!Succeeded(link->Set_Parent_Path(parent_path)))
			rc = E_UNEXPECTED;
	}

	return rc;
}


wchar_t* CPersistent_Chain_Configuration::Describe_GUID(const GUID& val, const scgms::NParameter_Type param_type, const scgms::CSignal_Description & signal_descriptors) const {
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