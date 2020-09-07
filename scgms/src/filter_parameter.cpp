#include "filter_parameter.h"

#include "../../../common/rtl/manufactory.h"
#include "../../../common/utils/string_utils.h"

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
	if ((read_interpreted == TRUE) && !mSystem_Variable_Name.empty()) {
		const char* sys = std::getenv(mSystem_Variable_Name.c_str());
		std::wstring value = sys ? Widen_Char(sys) : L"";
		*wstr = refcnt::WString_To_WChar_Container(value.c_str());
		return *wstr ? (sys ? S_OK : S_FALSE) : E_FAIL;
	}
		else return Get_Container(mWChar_Container, wstr);	
}

HRESULT IfaceCalling CFilter_Parameter::Set_WChar_Container(refcnt::wstr_container *wstr) {
	std::wstring tmp = WChar_Container_To_WString(wstr);
	mSystem_Variable_Name.clear();
	if (tmp.size() > 3) {
		if ((tmp[0] == '$') && (tmp[1] == L'(') && (tmp[tmp.size() - 1] == L')')) {
			mSystem_Variable_Name = Narrow_WString(tmp.substr(2, tmp.size() - 3));
		}
	}

	mWChar_Container = wstr;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_File_Path(refcnt::wstr_container** wstr) {
	filesystem::path result_path;

	if (mSystem_Variable_Name.empty()) {
		if (mWChar_Container->empty() == S_OK) {
			*wstr = nullptr;
			return S_FALSE;
		}

		result_path = refcnt::WChar_Container_To_WString(mWChar_Container.get());
	} else {
		const char* sys = std::getenv(mSystem_Variable_Name.c_str());
		if (!sys) {
			*wstr = nullptr;
			return E_NOT_SET;
		}

		result_path = sys;
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

	*wstr = refcnt::WString_To_WChar_Container(result_path.wstring().c_str());

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
	*value = mData.dbl;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Double(const double value) {
	mData.dbl = value;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Int64(int64_t *value) {
	*value = mData.int64;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Int64(const int64_t value) {
	mData.int64 = value;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Bool(BOOL *boolean) {
	*boolean = mData.boolean ? TRUE : FALSE;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Bool(const BOOL boolean) {
	mData.boolean = boolean != FALSE;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_GUID(GUID *id) {
	*id = mData.guid;
	return S_OK;
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
	clone->mSystem_Variable_Name = mSystem_Variable_Name;
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
	return E_NOTIMPL;
}

std::optional<std::wstring> CFilter_Parameter::Read_Variable(const wchar_t* name) {
	return std::wstring{};
}

HRESULT IfaceCalling create_filter_parameter(const scgms::NParameter_Type type, const wchar_t *config_name, scgms::IFilter_Parameter **parameter) {
	return Manufacture_Object<CFilter_Parameter, scgms::IFilter_Parameter>(parameter, type, config_name);
}

