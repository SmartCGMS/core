#include "configuration_link.h"

#include "../../../common/rtl/manufactory.h"

TEmbedded_Error CFilter_Configuration_Link::Initialize(const GUID& id) noexcept { 
	mID = id;
	return { S_OK, nullptr }; 
}

HRESULT IfaceCalling CFilter_Configuration_Link::Get_Filter_Id(GUID *id) noexcept {
	*id = mID;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Configuration_Link::Set_Parent_Path(const wchar_t* path) noexcept {
	if ((!path) || (*path == 0)) {
		mParent_Path.clear();
		return S_OK;
	};
	
	mParent_Path = path;

	HRESULT rc = S_OK;
	for (auto param : *this) {
		if (!Succeeded(param->Set_Parent_Path(path)))
			rc = E_UNEXPECTED;
	}

	return rc;
}

HRESULT IfaceCalling CFilter_Configuration_Link::Set_Variable(const wchar_t* name, const wchar_t* value) noexcept {
	if ((!name) || (*name == 0)) return E_INVALIDARG;

	HRESULT rc = S_OK;
	for (auto param : *this) {
		if (!Succeeded(param->Set_Variable(name, value)))
			rc = E_UNEXPECTED;
	}

	return rc;

}

HRESULT IfaceCalling CFilter_Configuration_Link::add(scgms::IFilter_Parameter** begin, scgms::IFilter_Parameter** end) noexcept {
	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>::add(begin, end);
	if (rc == S_OK)
		rc = Set_Parent_Path(mParent_Path.c_str());

	return rc;
}

HRESULT IfaceCalling create_filter_configuration_link(const GUID* id, scgms::IFilter_Configuration_Link** link) noexcept {
	return Manufacture_Object<CFilter_Configuration_Link>(link, *id).code;
}
