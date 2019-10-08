#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include "filter_parameter.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CFilter_Configuration_Link : public virtual refcnt::internal::CVector_Container<glucose::IFilter_Parameter*>, public virtual glucose::IFilter_Configuration_Link {
protected:
	const GUID mID;	
public:
	CFilter_Configuration_Link(const GUID &id);
	virtual ~CFilter_Configuration_Link() {};

	virtual HRESULT IfaceCalling Get_Filter_Id(GUID *id) override final;
};

#pragma warning( pop )

#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_filter_configuration_link(const GUID *id, glucose::IFilter_Configuration_Link **link);
#else
	extern "C" HRESULT IfaceCalling create_filter_configuration_link(const GUID *id, glucose::IFilter_Configuration_Link **link);
#endif