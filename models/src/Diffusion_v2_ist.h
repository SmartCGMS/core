#pragma once

#include "Diffusion_v2_blood.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance



class CDiffusion_v2_ist : public virtual CDiffusion_v2_blood {
protected:
	glucose::SSignal mBlood;
public:
	CDiffusion_v2_ist(glucose::WTime_Segment segment);
	virtual ~CDiffusion_v2_ist() {};

	//glucose::ISignal iface
	virtual HRESULT IfaceCalling Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
		const double *times, const double *levels, const size_t count, const size_t derivation_order) const final;	
};

#pragma warning( pop )