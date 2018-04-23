#pragma once

#include "../../../common/iface/ApproxIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"

#include <memory>
#include <thread>

#include <Eigen/Dense>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for computing model parameters
 */
class CLine_Approximator : public glucose::IApproximator, public virtual refcnt::CReferenced
{
	protected:
		glucose::SSignal mSignal;

		Eigen::Array<double, 1, Eigen::Dynamic> mInputTimes;
		Eigen::Array<double, 1, Eigen::Dynamic> mInputLevels;

		Eigen::Array<double, 1, Eigen::Dynamic> mSlopes;

		void Update();

	public:
		CLine_Approximator(glucose::SSignal signal, glucose::IApprox_Parameters_Vector* configuration);

		virtual HRESULT IfaceCalling GetLevels(const double* times, double* const levels, const size_t count, size_t derivation_order) override final;
};

#pragma warning( pop )