#pragma once

#include "../../../common/rtl/Common_Calculation.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance



class CDiffusion_v2_blood : public virtual CCommon_Calculation {
protected:
	glucose::SSignal mIst;
public:
	CDiffusion_v2_blood(glucose::WTime_Segment segment);
	virtual ~CDiffusion_v2_blood() {};

	//glucose::ISignal iface
	virtual HRESULT IfaceCalling Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
		const double* times, double* const levels, const size_t count, const size_t derivation_order) const;
	virtual HRESULT IfaceCalling Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const final;
};

#pragma warning( pop )