#pragma once

#include "../../../common/iface/ApproxIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for computing model parameters
 */
class CLine_Approximator : public glucose::IApproximator, public virtual refcnt::CReferenced {
	protected:
		glucose::WSignal mSignal;

		std::vector<double> mInputTimes, mInputLevels, mSlopes;

		bool Update();

	public:
		CLine_Approximator(glucose::WSignal signal, glucose::IApprox_Parameters_Vector* configuration);
		virtual ~CLine_Approximator() {};

		virtual HRESULT IfaceCalling GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) override;
};

#pragma warning( pop )
