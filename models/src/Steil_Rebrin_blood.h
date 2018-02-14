#pragma once

#include "../../../common/rtl/Common_Calculation.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance


class CSteil_Rebrin_blood : public virtual CCommon_Calculation {
protected:
	glucose::SSignal mIst;
	glucose::SSignal mCalibration;
public:
	CSteil_Rebrin_blood(glucose::WTime_Segment segment);
	virtual ~CSteil_Rebrin_blood() {};

	//glucose::ISignal iface
	virtual HRESULT IfaceCalling Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
		const double *times, const double *levels, const size_t count, const size_t derivation_order) const final;
	virtual HRESULT IfaceCalling Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const final;
};

#pragma warning( pop )