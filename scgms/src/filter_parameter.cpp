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
		return wstr_2_int(var_val.c_str(), ok);		
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
	mModel_Parameters = parameters;
	return S_OK;
}


HRESULT IfaceCalling CFilter_Parameter::Clone(scgms::IFilter_Parameter **deep_copy) {
	std::unique_ptr<CFilter_Parameter> clone = std::make_unique<CFilter_Parameter>(mType, mConfig_Name.c_str());
	clone->mVariable_Name = mVariable_Name;
	clone->mWChar_Container = refcnt::Copy_Container<wchar_t>(mWChar_Container.get());
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

HRESULT IfaceCalling create_filter_parameter(const scgms::NParameter_Type type, const wchar_t *config_name, scgms::IFilter_Parameter **parameter) {
	return Manufacture_Object<CFilter_Parameter, scgms::IFilter_Parameter>(parameter, type, config_name);
}