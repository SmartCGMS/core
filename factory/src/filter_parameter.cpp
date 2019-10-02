#include "filter_parameter.h"

#include "../../../common/rtl/manufactory.h"

CFilter_Parameter::CFilter_Parameter(const glucose::NParameter_Type type, const wchar_t *config_name) : mType(type), mConfig_Name(config_name) {
	//
}

HRESULT IfaceCalling CFilter_Parameter::Get_Type(glucose::NParameter_Type *type) {
	*type = mType;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Config_Name(wchar_t **config_name) {
	(*config_name) = const_cast<wchar_t*>(mConfig_Name.c_str());
	return S_OK;
}


HRESULT IfaceCalling CFilter_Parameter::Get_WChar_Container(refcnt::wstr_container **wstr) {
	return Get_Container(mWChar_Container, wstr);	
}

HRESULT IfaceCalling CFilter_Parameter::Set_WChar_Container(refcnt::wstr_container *wstr) {
	mWChar_Container = wstr;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Get_Time_Segment_Id_Container(glucose::time_segment_id_container **ids) {
	return Get_Container(mTime_Segment_ID, ids);
}

HRESULT IfaceCalling CFilter_Parameter::Set_Time_Segment_Id_Container(glucose::time_segment_id_container *ids) {
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

HRESULT IfaceCalling CFilter_Parameter::Get_Bool(uint8_t *boolean) {
	*boolean = mData.boolean ? static_cast<uint8_t>(-1) : 0;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Parameter::Set_Bool(const uint8_t boolean) {
	mData.boolean = boolean != 0;
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

HRESULT IfaceCalling CFilter_Parameter::Get_Model_Parameters(glucose::IModel_Parameter_Vector **parameters) {
	return Get_Container(mModel_Parameters, parameters);
}

HRESULT IfaceCalling CFilter_Parameter::Set_Model_Parameters(glucose::IModel_Parameter_Vector *parameters) {
	mModel_Parameters = parameters;
	return S_OK;
}


HRESULT IfaceCalling create_filter_parameter(const glucose::NParameter_Type type, const wchar_t *config_name, glucose::IFilter_Parameter **parameter) {
	return Manufacture_Object<CFilter_Parameter, glucose::IFilter_Parameter>(parameter, type, config_name);
}