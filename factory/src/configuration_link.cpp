#include "configuration_link.h"

#include "../../../common/rtl/manufactory.h"

CFilter_Configuration_Link::CFilter_Configuration_Link(const GUID &id) : mID(id) {
	//
}

HRESULT IfaceCalling CFilter_Configuration_Link::Get_Filter_Id(GUID *id) {
	*id = mID;
	return S_OK;
}

HRESULT IfaceCalling create_filter_configuration_link(const GUID *id, glucose::IFilter_Configuration_Link **link) {
	return Manufacture_Object<CFilter_Configuration_Link>(link, *id);
}