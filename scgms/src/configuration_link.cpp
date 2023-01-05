#include "configuration_link.h"

#include "../../../common/rtl/manufactory.h"

CFilter_Configuration_Link::CFilter_Configuration_Link(const GUID &id) : mID(id) {
//	mParent_Path = std::make_unique<std::wstring>();
}

HRESULT IfaceCalling CFilter_Configuration_Link::Get_Filter_Id(GUID *id) {
	*id = mID;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Configuration_Link::Set_Parent_Path(const wchar_t* path) {
	if ((!path) || (*path == 0)) {
		mParent_Path.clear();
		return S_OK;
	};
	
	mParent_Path = path;

	HRESULT rc = S_OK;
	for (auto param : mData) {
		if (!Succeeded(param->Set_Parent_Path(mParent_Path.c_str())))
			rc = E_UNEXPECTED;
	}

	return rc;
}

HRESULT IfaceCalling CFilter_Configuration_Link::Set_Variable(const wchar_t* name, const wchar_t* value) {
	if ((!name) || (*name == 0)) return E_INVALIDARG;
	if (name == CFilter_Parameter::mUnused_Variable_Name) return TYPE_E_AMBIGUOUSNAME;

	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>::empty();
	if (rc == S_FALSE) {
		for (auto param : mData) {
			if (!Succeeded(param->Set_Variable(name, value)))
				rc = E_UNEXPECTED;
		}
	}

	return rc;

}

HRESULT IfaceCalling CFilter_Configuration_Link::add(scgms::IFilter_Parameter** begin, scgms::IFilter_Parameter** end) {
	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>::add(begin, end);
	if (rc == S_OK)
		rc = Set_Parent_Path(mParent_Path.c_str());

	return rc;
}

HRESULT IfaceCalling create_filter_configuration_link(const GUID* id, scgms::IFilter_Configuration_Link** link) {
	return Manufacture_Object<CFilter_Configuration_Link>(link, *id);
}
