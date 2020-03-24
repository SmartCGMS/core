#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include "filter_parameter.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CFilter_Configuration_Link : public virtual refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>, public virtual scgms::IFilter_Configuration_Link {
protected:
	const GUID mID;	
public:
	CFilter_Configuration_Link(const GUID &id);
	virtual ~CFilter_Configuration_Link() {};

	virtual HRESULT IfaceCalling Get_Filter_Id(GUID *id) override final;
};

#pragma warning( pop )

extern "C" HRESULT IfaceCalling create_filter_configuration_link(const GUID *id, scgms::IFilter_Configuration_Link **link);
