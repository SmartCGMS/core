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


#include "filter_parameter.h"
#include "filters.h"

#include "../../../common/rtl/manufactory.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <fstream>

const std::wstring CFilter_Parameter::mUnused_Variable_Name = rsUnused_Variable_Name;

double str_2_rat_dbl(const std::wstring& str, bool& converted_ok) {
	double value = str_2_dbl(str.c_str(), converted_ok);
	if (!converted_ok)
		value = Default_Str_To_Rat_Time(str.c_str(), converted_ok);
	return value;
}

HRESULT IfaceCalling create_filter_parameter(const scgms::NParameter_Type type, const wchar_t* config_name, scgms::IFilter_Parameter** parameter) {
	return Manufacture_Object<CFilter_Parameter, scgms::IFilter_Parameter>(parameter, type, config_name);
}


CFilter_Parameter::CFilter_Parameter(const scgms::NParameter_Type type, const wchar_t *config_name) : mType(type), mConfig_Name(config_name) {
	//
}

HRESULT IfaceCalling CFilter_Parameter::Get_Type(scgms::NParameter_Type *type) {
	*type = mType;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Config_Name(wchar_t **config_name) {
	(*config_name) = const_cast<wchar_t*>(mConfig_Name.c_str());
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_WChar_Container(refcnt::wstr_container **wstr, BOOL read_interpreted) {
	auto [rc, converted] = to_string(read_interpreted == TRUE);

	if (rc == S_OK)
		*wstr = refcnt::WString_To_WChar_Container(converted.c_str());
	return rc;	
}

HRESULT IfaceCalling CFilter_Parameter::Set_WChar_Container(refcnt::wstr_container *wstr) {
	std::wstring tmp = WChar_Container_To_WString(wstr);
	if (from_string(mType, tmp.c_str())) {
		mWChar_Container = refcnt::make_shared_reference_ext<decltype(mWChar_Container), refcnt::wstr_container>(wstr, true);
		return S_OK;
	}
	else
		return E_INVALIDARG;
}

HRESULT IfaceCalling CFilter_Parameter::Get_File_Path(refcnt::wstr_container** wstr) {
	filesystem::path result_path;

	if (mVariable_Name.empty()) {
		if (mWChar_Container->empty() == S_OK) {
			*wstr = nullptr;
			return S_FALSE;
		}

		result_path = refcnt::WChar_Container_To_WString(mWChar_Container.get());
	} else {
		auto [rc, var_val] = Evaluate_Variable(mVariable_Name);

		if (!Succeeded(rc)) {
			*wstr = nullptr;
			return rc;
		}

		result_path = var_val;
	}

	std::wstring converted_path = Make_Absolute_Path(result_path);
	*wstr = refcnt::WString_To_WChar_Container(converted_path.c_str());

	return S_OK;
}

std::wstring CFilter_Parameter::Make_Absolute_Path(filesystem::path src_path) {
	if (src_path.is_relative()) {
		std::error_code ec;
		filesystem::path relative_part = src_path;
		src_path = filesystem::canonical(mParent_Path / relative_part, ec);
		if (ec) {
			src_path = filesystem::weakly_canonical(mParent_Path / relative_part, ec);
			if (ec)
				src_path = mParent_Path / relative_part;
		}
	}


	src_path = src_path.make_preferred();	//we know that make_preferred fails sometimes
	std::wstring converted_path = src_path.wstring();
	converted_path = Ensure_Uniform_Dir_Separator(converted_path);

	return converted_path;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Parent_Path(const wchar_t* parent_path) {
	if ((!parent_path) || (*parent_path == 0)) return E_INVALIDARG;

	mParent_Path = parent_path;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Time_Segment_Id_Container(scgms::time_segment_id_container **ids) {
	if (mTime_Segment_ID) {
		HRESULT rc = Update_Container_By_Vars<int64_t>(mTime_Segment_ID, str_2_int);
		return Succeeded(rc) ? Get_Container(mTime_Segment_ID, ids) : rc;
	}
	else
		return S_FALSE;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Time_Segment_Id_Container(scgms::time_segment_id_container *ids) {
	//by setting this to max, we effectively discard any nested variable and do not need to perform any additinal action
	mFirst_Array_Var_idx = std::numeric_limits<size_t>::max();
	mVariable_Name.clear();

	mTime_Segment_ID = refcnt::make_shared_reference_ext<decltype(mTime_Segment_ID), scgms::time_segment_id_container>(ids, true);
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Double(double *value) {
	return Get_Value<double>(value, [this]() {return mData.dbl; }, [](const std::wstring& var_val, bool& ok) {
			return str_2_rat_dbl(var_val, ok);
		},
		std::numeric_limits<double>::quiet_NaN());
}

HRESULT IfaceCalling CFilter_Parameter::Set_Double(const double value) {
	mVariable_Name.clear();
	mData.dbl = value;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Int64(int64_t *value) {
	return Get_Value<int64_t>(value, [this]() {return mData.int64; }, [](const std::wstring& var_val, bool& ok) {
		return str_2_int(var_val.c_str(), ok);		
	}, std::numeric_limits<int64_t>::max());
}

HRESULT IfaceCalling CFilter_Parameter::Set_Int64(const int64_t value) {
	mVariable_Name.clear();
	mData.int64 = value;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Bool(BOOL *boolean) {
	return Get_Value<BOOL>(boolean, [this]() {return mData.boolean ? TRUE : FALSE; }, [](const std::wstring& var_val, bool& ok) {
		const bool val = str_2_bool(var_val, ok);
		return val ? TRUE : FALSE;
	}, false);
}

HRESULT IfaceCalling CFilter_Parameter::Set_Bool(const BOOL boolean) {
	mVariable_Name.clear();
	mData.boolean = boolean != FALSE;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_GUID(GUID *id) {
	return Get_Value<GUID>(id, [this]() {return mData.guid; }, [](const std::wstring& var_val, bool& ok) {
		return WString_To_GUID(var_val, ok);
	}, Invalid_GUID);
}

HRESULT IfaceCalling CFilter_Parameter::Set_GUID(const GUID *id) {
	mVariable_Name.clear();
	mData.guid = *id;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Model_Parameters(scgms::IModel_Parameter_Vector **parameters) {
	if (mModel_Parameters) {
		HRESULT rc = Update_Container_By_Vars<double>(mModel_Parameters, str_2_rat_dbl);
		return Succeeded(rc) ? Get_Container(mModel_Parameters, parameters) : rc;
	}
	else
		return E_NOT_SET;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Model_Parameters(scgms::IModel_Parameter_Vector *parameters) {
	//by setting this to max, we effectively discard any nested variable and do not need to perform any additinal action
	mFirst_Array_Var_idx = std::numeric_limits<size_t>::max();
	mVariable_Name.clear();
	mArray_Vars.clear();

	mModel_Parameters = refcnt::make_shared_reference_ext<decltype(mModel_Parameters), scgms::IModel_Parameter_Vector>(parameters, true);
	return S_OK;
}


HRESULT IfaceCalling CFilter_Parameter::Clone(scgms::IFilter_Parameter **deep_copy) {
	std::unique_ptr<CFilter_Parameter> clone = std::make_unique<CFilter_Parameter>(mType, mConfig_Name.c_str());
	clone->mVariable_Name = mVariable_Name;
	clone->mWChar_Container = refcnt::Copy_Container_shared<wchar_t, decltype(mWChar_Container)>(mWChar_Container.get());
	clone->mArray_Vars = mArray_Vars;
	clone->mFirst_Array_Var_idx = mFirst_Array_Var_idx;
	clone->mTime_Segment_ID = refcnt::Copy_Container_shared<int64_t, decltype(mTime_Segment_ID)>(mTime_Segment_ID.get());
	clone->mModel_Parameters = refcnt::Copy_Container_shared<double, decltype(mModel_Parameters)>(mModel_Parameters.get());
	clone->mData = mData;

	clone->mParent_Path = mParent_Path;
	clone->mDeferred_Path_Or_Var = mDeferred_Path_Or_Var;

	(*deep_copy) = static_cast<scgms::IFilter_Parameter*>(clone.get());
	(*deep_copy)->AddRef();
	clone.release();

	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Variable(const wchar_t* name, const wchar_t* value) {
	if (name == CFilter_Parameter::mUnused_Variable_Name) return TYPE_E_AMBIGUOUSNAME;
	mNon_OS_Variables[name] = value;
	return S_OK;
}

std::tuple<HRESULT, std::wstring> CFilter_Parameter::Evaluate_Variable(const std::wstring& var_name) {

	if (var_name == CFilter_Parameter::mUnused_Variable_Name)
		return { S_FALSE, std::wstring{} };	//valid text for an unused option


	//try our variables as they may hide the OS ones to actually customize them
	{
		auto iter = mNon_OS_Variables.find(var_name);
		if (iter != mNon_OS_Variables.end())
			return std::tuple<bool, std::wstring>{S_OK, iter->second};
	}

	//try OS variables
	{
		std::string ansi = Narrow_WString(var_name);
		const char* sys = std::getenv(ansi.c_str());
		if (sys)
			return std::tuple<bool, std::wstring>{S_OK, Widen_Char(sys)};
	}
		

	return { E_NOT_SET, std::wstring{} };	//not found at all
}


std::wstring CFilter_Parameter::Resolve_Deferred_Path() {
	std::wstring effective_deferred_path = mDeferred_Path_Or_Var;
	HRESULT rc = E_UNEXPECTED;

	//deferred path could still be an OS-variable, which needs to be resolved
	const auto [is_var, var_name] = scgms::Is_Variable_Name(mDeferred_Path_Or_Var);
	if (is_var) {
		std::tie(rc, effective_deferred_path) = Evaluate_Variable(var_name);
	}
	else
		rc = S_OK;

	return Succeeded(rc) ? Make_Absolute_Path(effective_deferred_path) : std::wstring{};
}

bool CFilter_Parameter::from_string(const scgms::NParameter_Type desired_type, const wchar_t* str) {
	wchar_t* effective_str = const_cast<wchar_t*>(str);
	std::wstring possibly_deferred_content = L"";

	//check for a possibly deferred file
	bool is_deferred = false;
	std::tie(is_deferred, mDeferred_Path_Or_Var) = Is_Deferred_Parameter(str);
	if (is_deferred) {
		auto effective_deferred_path = Resolve_Deferred_Path();
		HRESULT deferred_success = E_UNEXPECTED;
		std::tie(deferred_success, possibly_deferred_content) = Load_From_File(effective_deferred_path.c_str());
		if (Succeeded(deferred_success))
			effective_str = const_cast<wchar_t*>(possibly_deferred_content.c_str());
	}
	else
		mDeferred_Path_Or_Var.clear();


	//when converting an array, each variable reference represents just one value
	if ((desired_type != scgms::NParameter_Type::ptDouble_Array) && (desired_type != scgms::NParameter_Type::ptInt64_Array)) {

		auto [is_var, var_name] = scgms::Is_Variable_Name(effective_str);
		if (is_var) {
			mVariable_Name = var_name;
			return true;
		}
		else
			mVariable_Name.clear();
	} else
		mVariable_Name.clear();	//just for sanity, as array uses mArray_Vars

	bool valid = false;

	switch (desired_type) {

		case scgms::NParameter_Type::ptWChar_Array:		
			mWChar_Container = refcnt::make_shared_reference_ext<decltype(mWChar_Container), refcnt::wstr_container>(refcnt::WString_To_WChar_Container(effective_str), false);
			valid = true;
			break;

		case scgms::NParameter_Type::ptInt64_Array:
			{
				mTime_Segment_ID = refcnt::make_shared_reference_ext<decltype(mTime_Segment_ID), scgms::time_segment_id_container>(Parse_Array_String<int64_t, scgms::time_segment_id_container>(effective_str, str_2_int), false);
				valid = mTime_Segment_ID.operator bool();		
			}
			break;

		case scgms::NParameter_Type::ptRatTime:	
		case scgms::NParameter_Type::ptDouble:
		{
			double val = str_2_rat_dbl(effective_str, valid);		
			if (valid)
				mData.dbl = val;
		}
		break;

		case scgms::NParameter_Type::ptInt64:
		case scgms::NParameter_Type::ptSubject_Id:
		{
			int64_t val = str_2_int(effective_str, valid);
			if (valid)
				mData.int64 = val;
		}
		break;

		case scgms::NParameter_Type::ptBool:
		{
			bool val = str_2_bool(effective_str, valid);
			if (valid)
				mData.boolean = val;
		}
			break;

		case scgms::NParameter_Type::ptSignal_Model_Id:
		case scgms::NParameter_Type::ptDiscrete_Model_Id:
		case scgms::NParameter_Type::ptMetric_Id:
		case scgms::NParameter_Type::ptModel_Produced_Signal_Id:
		case scgms::NParameter_Type::ptSignal_Id:
		case scgms::NParameter_Type::ptSolver_Id:
		{
			GUID tmp_guid = WString_To_GUID(effective_str, valid);
			if (!valid) {
				//not found, but let's try to find the signal as if the string is its name
				tmp_guid = resolve_signal_by_name(effective_str, valid);
			}

			if (valid) 
				mData.guid = tmp_guid;
			
			
		}
		break;

		case scgms::NParameter_Type::ptDouble_Array:
			{
			mModel_Parameters =  refcnt::make_shared_reference_ext<decltype(mModel_Parameters), scgms::IModel_Parameter_Vector>( Parse_Array_String<double, scgms::IModel_Parameter_Vector>(effective_str, str_2_rat_dbl), false);
			valid = mModel_Parameters.operator bool();
		}
		break;

		default:
			valid = false;
	} //switch (desc.parameter_type[i])	{

	return valid;
}

std::tuple<HRESULT, std::wstring> CFilter_Parameter::to_string(bool read_interpreted) {

	std::wstring converted;
	HRESULT rc = S_OK;
	
	auto convert_scalar = [&]() {
		if (mVariable_Name.empty()) {
			//always behave as read_interpereted = true

			switch (mType) {
				case scgms::NParameter_Type::ptWChar_Array:
					converted = refcnt::WChar_Container_To_WString(mWChar_Container.get());
					break;

				case scgms::NParameter_Type::ptRatTime:
					converted = Rat_Time_To_Default_WStr(mData.dbl);
					break;

				case scgms::NParameter_Type::ptDouble:
					converted = dbl_2_wstr(mData.dbl);
					break;

				case scgms::NParameter_Type::ptInt64:
				case scgms::NParameter_Type::ptSubject_Id:
					converted = std::to_wstring(mData.int64);
					break;

				case scgms::NParameter_Type::ptBool:
					converted = mData.boolean ? L"true" : L"false";
					break;

				case scgms::NParameter_Type::ptSignal_Model_Id:
				case scgms::NParameter_Type::ptDiscrete_Model_Id:
				case scgms::NParameter_Type::ptMetric_Id:
				case scgms::NParameter_Type::ptModel_Produced_Signal_Id:
				case scgms::NParameter_Type::ptSignal_Id:
				case scgms::NParameter_Type::ptSolver_Id:
					converted = GUID_To_WString(mData.guid);
				break;

				default: break;
			} //switch (source_type) {

		}
		else {
			if (read_interpreted) 
				std::tie(rc, converted) = Evaluate_Variable(mVariable_Name);
			 else 
				converted = L"$(" + mVariable_Name + L")";
			//and that's all
		}
		
	};

	switch (mType) {

		case scgms::NParameter_Type::ptDouble_Array: 
			std::tie(rc, converted) = Array_To_String<double>(mModel_Parameters.get(), read_interpreted);
			break;

		case scgms::NParameter_Type::ptInt64_Array:
			std::tie(rc, converted) = Array_To_String<int64_t>(mTime_Segment_ID.get(), read_interpreted);
			break;

		default: convert_scalar();
			break;
	}

	if (rc != S_OK)
		converted.clear();

	if ((rc == S_OK) && (!mDeferred_Path_Or_Var.empty()) && (read_interpreted == false)) {
		//we actually have to save the content to a deferred file
		auto effective_deffered_path = Resolve_Deferred_Path();
		rc = Save_To_File(converted, effective_deffered_path.c_str());

		//file has been saved succesfully => write the deferred file path
		converted = mDeferred_Magic_String_Prefix;
		converted.append(L" ");
		converted.append(mDeferred_Path_Or_Var);
		converted.append(mDeferred_Magic_String_Postfix);
	}

	return std::tuple<HRESULT, std::wstring>{rc, converted};
}

std::tuple<bool, std::wstring> CFilter_Parameter::Is_Deferred_Parameter(const wchar_t* str_value) {
	const size_t len = wcslen(str_value);
	if (len < wcslen(mDeferred_Magic_String_Prefix) + wcslen(mDeferred_Magic_String_Postfix) + 1)
		return { false, L"" };	//just does not fit

	if (wmemcmp(str_value, mDeferred_Magic_String_Prefix, wcslen(mDeferred_Magic_String_Prefix)) != 0)
		return { false, L"" };	//unrecognized prefix


	if (str_value[len - 1] != mDeferred_Magic_String_Postfix[0])
		return { false, L"" };	//malformed

	std::wstring deferred_path{ str_value + wcslen(mDeferred_Magic_String_Prefix),
						 str_value + len - wcslen(mDeferred_Magic_String_Postfix) };

	return { true, trim(deferred_path) };
}

std::tuple<HRESULT, std::wstring> CFilter_Parameter::Load_From_File(const wchar_t* path) {
	std::wifstream src_file;
	src_file.open(path);

	if (src_file.is_open()) {
		std::vector<wchar_t> buf;
		buf.assign(std::istreambuf_iterator<wchar_t>(src_file), std::istreambuf_iterator<wchar_t>());
		// fix valgrind's "Conditional jump or move depends on uninitialised value(s)"
		// although we are sending proper length, SimpleIni probably reaches one byte further and reads uninitialized memory
		buf.push_back(0);
		return { S_OK, std::wstring{ buf.begin(), buf.end() } };
	}
	else
		return { S_FALSE, L"" };	//empty file is like an empty line
}

HRESULT CFilter_Parameter::Save_To_File(const std::wstring& text, const wchar_t* path) {
	std::wofstream dst_file;
	dst_file.open(path);

	if (dst_file.is_open()) {
		dst_file << text;
		return S_OK;
	}
	else
		return MK_E_CANTOPENFILE;
}
