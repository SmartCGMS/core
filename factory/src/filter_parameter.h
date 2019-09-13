#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CFilter_Parameter : public virtual glucose::IFilter_Parameter, public virtual refcnt::CReferenced {
protected:
	const glucose::NParameter_Type mType;
	const std::wstring mConfig_Name;

	//the following SReferenced variables are not part of the union to prevent memory corruption
	refcnt::SReferenced<refcnt::wstr_container> mWChar_Container;
	refcnt::SReferenced<glucose::time_segment_id_container> mTime_Segment_ID;
	refcnt::SReferenced<glucose::IModel_Parameter_Vector> mModel_Parameters;

	union {	
		double dbl;
		int64_t int64;
		bool boolean;
		GUID guid;		
	} mData;

	template <typename TSRC, typename TDST>
	HRESULT Get_Container(TSRC src, TDST dst) {
		if (src) {
			*dst = src.get();
			(*dst)->AddRef();
			return S_OK;
		}
		else {
			*dst = nullptr;
			return S_FALSE;
		}
	}
public:
	CFilter_Parameter(const glucose::NParameter_Type type, const wchar_t *config_name);
	virtual ~CFilter_Parameter() {};

	virtual HRESULT IfaceCalling Get_Type(glucose::NParameter_Type *type) override final;
	virtual HRESULT IfaceCalling Get_Config_Name(wchar_t const **config_name) override final;

	//read-write
	virtual HRESULT IfaceCalling Get_WChar_Container(refcnt::wstr_container **wstr) override final;
	virtual HRESULT IfaceCalling Set_WChar_Container(refcnt::wstr_container *wstr) override final;

	virtual HRESULT IfaceCalling Get_Time_Segment_Id_Container(glucose::time_segment_id_container **ids) override final;
	virtual HRESULT IfaceCalling Set_Time_Segment_Id_Container(glucose::time_segment_id_container *ids) override final;

	virtual HRESULT IfaceCalling Get_Double(double *value) override final;
	virtual HRESULT IfaceCalling Set_Double(const double value) override final;

	virtual HRESULT IfaceCalling Get_Int64(int64_t *value) override final;
	virtual HRESULT IfaceCalling Set_Int64(const int64_t value) override final;

	virtual HRESULT IfaceCalling Get_Bool(uint8_t *boolean) override final;
	virtual HRESULT IfaceCalling Set_Bool(const uint8_t boolean) override final;

	virtual HRESULT IfaceCalling Get_GUID(GUID *id) override final;
	virtual HRESULT IfaceCalling Set_GUID(const GUID *id) override final;

	virtual HRESULT IfaceCalling Get_Model_Parameters(glucose::IModel_Parameter_Vector **parameters) override final;
	virtual HRESULT IfaceCalling Set_Model_Parameters(glucose::IModel_Parameter_Vector *parameters) override final;
};

#pragma warning( pop )