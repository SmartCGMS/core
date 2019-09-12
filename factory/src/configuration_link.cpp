#include "configuration_link.h"

CFilter_Configuration_Link::CFilter_Configuration_Link(const GUID &id) : mID(id) {
	//
}

HRESULT IfaceCalling CFilter_Configuration_Link::Get_Filter_Id(GUID *id) {
	*id = mID;
	return S_OK;
}