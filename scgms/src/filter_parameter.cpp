#include "filter_parameter.h"

#include "../../../common/rtl/manufactory.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/rtl/rattime.h"

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
	if ((read_interpreted == TRUE) && !mVariable_Name.empty()) {

		auto [var_set, var_val] = Evaluate_Variable(mVariable_Name);
		if (!var_set)
			var_val = L"";		

		*wstr = refcnt::WString_To_WChar_Container(var_val.c_str());
		return *wstr ? (var_set ? S_OK : S_FALSE) : E_FAIL;
	}
		else return Get_Container(mWChar_Container, wstr);	
}

HRESULT IfaceCalling CFilter_Parameter::Set_WChar_Container(refcnt::wstr_container *wstr) {
	std::wstring tmp = WChar_Container_To_WString(wstr);
	mVariable_Name.clear();

	auto [is_var, var_name] = scgms::Is_Variable_Name(tmp);
	if (is_var)
		mVariable_Name = var_name;

	mWChar_Container = wstr;
	return S_OK;
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
		auto [var_set, var_val] = Evaluate_Variable(mVariable_Name);

		if (!var_set) {
			*wstr = nullptr;
			return E_NOT_SET;
		}

		result_path = var_val;
	}

	if (result_path.is_relative()) {		
		std::error_code ec;
		filesystem::path relative_part = result_path;
		result_path = filesystem::canonical(mParent_Path / relative_part, ec);
		if (ec) {
			result_path = filesystem::weakly_canonical(mParent_Path / relative_part, ec);
			if (ec)
				result_path = mParent_Path / relative_part;
		}
	}

	result_path = result_path.make_preferred();
        const auto converted_path = result_path.wstring();
        *wstr = refcnt::WString_To_WChar_Container(converted_path.c_str());

	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Parent_Path(const wchar_t* parent_path) {
	if ((!parent_path) || (*parent_path == 0)) return E_INVALIDARG;

	mParent_Path = parent_path;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Time_Segment_Id_Container(scgms::time_segment_id_container **ids) {
	return Get_Container(mTime_Segment_ID, ids);
}

HRESULT IfaceCalling CFilter_Parameter::Set_Time_Segment_Id_Container(scgms::time_segment_id_container *ids) {
	//by setting this to max, we effectively discard any nested variable and do not need to perform any additinal action
	mFirst_Array_Var_idx = std::numeric_limits<size_t>::max();

	mTime_Segment_ID = ids;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Double(double *value) {
	return Get_Value<double>(value, [this]() {return mData.dbl; }, [](const std::wstring& var_val, bool& ok) {
			return str_2_rat_dbl(var_val, ok);
		},
		std::numeric_limits<double>::quiet_NaN());
}

HRESULT IfaceCalling CFilter_Parameter::Set_Double(const double value) {
	mData.dbl = value;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Int64(int64_t *value) {
	return Get_Value<int64_t>(value, [this]() {return mData.int64; }, [](const std::wstring& var_val, bool& ok) {
		return str_2_int(var_val.c_str(), ok);		
	}, std::numeric_limits<int64_t>::max());
}

HRESULT IfaceCalling CFilter_Parameter::Set_Int64(const int64_t value) {
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
	mData.boolean = boolean != FALSE;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_GUID(GUID *id) {
	return Get_Value<GUID>(id, [this]() {return mData.guid; }, [](const std::wstring& var_val, bool& ok) {
		return WString_To_GUID(var_val, ok);
	}, Invalid_GUID);
}

HRESULT IfaceCalling CFilter_Parameter::Set_GUID(const GUID *id) {	
	mData.guid = *id;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Model_Parameters(scgms::IModel_Parameter_Vector **parameters) {
	return Get_Container(mModel_Parameters, parameters);
}

HRESULT IfaceCalling CFilter_Parameter::Set_Model_Parameters(scgms::IModel_Parameter_Vector *parameters) {
	//by setting this to max, we effectively discard any nested variable and do not need to perform any additinal action
	mModel_Parameters = parameters;
	return S_OK;
}


HRESULT IfaceCalling CFilter_Parameter::Clone(scgms::IFilter_Parameter **deep_copy) {
	std::unique_ptr<CFilter_Parameter> clone = std::make_unique<CFilter_Parameter>(mType, mConfig_Name.c_str());	
	clone->mVariable_Name = mVariable_Name;
	clone->mWChar_Container = refcnt::Copy_Container<wchar_t>(mWChar_Container.get());
	clone->mArray_Vars = mArray_Vars;
	clone->mFirst_Array_Var_idx = mFirst_Array_Var_idx;
	clone->mTime_Segment_ID = refcnt::Copy_Container<int64_t>(mTime_Segment_ID.get());
	clone->mModel_Parameters = refcnt::Copy_Container<double>(mModel_Parameters.get());
	clone->mData = mData;

	clone->mParent_Path = mParent_Path;

	(*deep_copy) = static_cast<scgms::IFilter_Parameter*>(clone.get());
	(*deep_copy)->AddRef();
	clone.release();

	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Variable(const wchar_t* name, const wchar_t* value) {
	mNon_OS_Variables[name] = value;
	return S_OK;
}

std::tuple<bool, std::wstring> CFilter_Parameter::Evaluate_Variable(const std::wstring& var_name) {

	//try our variables as they may hide the OS ones to actually customize them
	{
		auto iter = mNon_OS_Variables.find(var_name);
		if (iter != mNon_OS_Variables.end())
			return std::tuple<bool, std::wstring>{true, iter->second};
	}

	//try OS variables
	{
		std::string ansi = Narrow_WString(var_name);
		const char* sys = std::getenv(ansi.c_str());
		if (sys)
			return std::tuple<bool, std::wstring>{true, Widen_Char(sys)};
	}
		

	return { false, std::wstring{} };	//not found at all
}


bool CFilter_Parameter::from_string(const scgms::NParameter_Type desired_type, const wchar_t* str) {
	auto [is_var, var_name] = scgms::Is_Variable_Name(str);
	if (is_var) {
		mVariable_Name = var_name;
		return true;
	}
	else
		mVariable_Name.clear();

	bool valid = false;

	switch (desired_type) {

		case scgms::NParameter_Type::ptWChar_Array:		
			mWChar_Container = refcnt::WString_To_WChar_Container(str);
			valid = true;
			break;

		case scgms::NParameter_Type::ptInt64_Array:
			{
				mTime_Segment_ID = Parse_Array_String<int64_t, scgms::time_segment_id_container>(str, str_2_int);
				valid = mTime_Segment_ID.operator bool();		
			}
			break;

		case scgms::NParameter_Type::ptRatTime:	
		case scgms::NParameter_Type::ptDouble:
		{
			double val = str_2_rat_dbl(str, valid);		
			if (valid)
				mData.dbl = val;
		}
		break;

		case scgms::NParameter_Type::ptInt64:
		case scgms::NParameter_Type::ptSubject_Id:
		{
			int64_t val = str_2_int(str, valid);
			if (valid)
				mData.int64 = val;			
		}
		break;

		case scgms::NParameter_Type::ptBool:
		{
			bool val = str_2_bool(str, valid);
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
			const GUID tmp_guid = WString_To_GUID(str, valid);
			if (valid)
				mData.guid = tmp_guid;
		}
		break;

		case scgms::NParameter_Type::ptDouble_Array:
			{
			mModel_Parameters = Parse_Array_String<double, scgms::IModel_Parameter_Vector>(str, str_2_rat_dbl);
			valid = mModel_Parameters.operator bool();
		}
		break;

		default:
			valid = false;
	} //switch (desc.parameter_type[i])	{

	return valid;
}