#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include "filter_parameter.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CFilter_Configuration_Link : public virtual refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>, public virtual scgms::IFilter_Configuration_Link {
protected:
	GUID mID;	
	std::wstring mParent_Path;	//for resolving relative paths
public:
	CFilter_Configuration_Link() noexcept = default;
	virtual ~CFilter_Configuration_Link() noexcept {};

	TEmbedded_Error Initialize(const GUID& id) noexcept;

	virtual HRESULT IfaceCalling add(scgms::IFilter_Parameter* *begin, scgms::IFilter_Parameter* *end) noexcept override;

	virtual HRESULT IfaceCalling Get_Filter_Id(GUID *id) noexcept override final;
	virtual HRESULT IfaceCalling Set_Parent_Path(const wchar_t* path) noexcept override final;
	virtual HRESULT IfaceCalling Set_Variable(const wchar_t* name, const wchar_t* value) noexcept override final;
};

#pragma warning( pop )

extern "C" HRESULT IfaceCalling create_filter_configuration_link(const GUID *id, scgms::IFilter_Configuration_Link **link) noexcept;
