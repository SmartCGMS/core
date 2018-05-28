#pragma once

#include "../../../common/rtl/ApproxLib.h"
#include "../../../common/rtl/referencedImpl.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CMeasured_Signal : public virtual glucose::ISignal, public virtual refcnt::CReferenced
{
	protected:
		std::vector<double> mTimes;
		std::vector<double> mLevels;

		glucose::SApproximator mApprox;

	public:
		CMeasured_Signal();
		virtual ~CMeasured_Signal() {};

		virtual HRESULT IfaceCalling Get_Discrete_Levels(double* const times, double* const levels, const size_t count, size_t *filled) const override final;
		virtual HRESULT IfaceCalling Get_Discrete_Bounds(glucose::TBounds *bounds, size_t *level_count) const override final;
		virtual HRESULT IfaceCalling Add_Levels(const double *times, const double *levels, const size_t count) override final;

		virtual HRESULT IfaceCalling Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const override final;
		virtual HRESULT IfaceCalling Get_Default_Parameters(glucose::IModel_Parameter_Vector *parameters) const override final;
};

#pragma warning( pop )
